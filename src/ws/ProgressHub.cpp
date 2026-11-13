#include "ws/ProgressHub.h"
#include "core/Logger.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <openssl/evp.h>

namespace el::ws
{
    namespace
    {
        constexpr const char* kWebSocketGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

        std::string base64(const unsigned char* data, std::size_t size)
        {
            const int maxLen = 4 * ((static_cast<int>(size) + 2) / 3) + 1;
            std::vector<char> buffer(static_cast<std::size_t>(maxLen), '\0');
            const int written = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(buffer.data()),
                                                data, static_cast<int>(size));
            if (written <= 0) return {};
            return std::string(buffer.data(), static_cast<std::size_t>(written));
        }

        std::string acceptKey(const std::string& clientKey)
        {
            const std::string input = clientKey + kWebSocketGuid;
            std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
            unsigned int digestLen = 0;
            if (EVP_Digest(input.data(), input.size(), digest.data(), &digestLen,
                           EVP_sha1(), nullptr) != 1)
            {
                return {};
            }
            return base64(digest.data(), digestLen);
        }

        std::string headerValue(const std::string& headers, const std::string& name)
        {
            std::size_t pos = 0;
            while (pos < headers.size())
            {
                const std::size_t end = headers.find("\r\n", pos);
                const std::size_t lineEnd = end == std::string::npos ? headers.size() : end;
                const std::string line = headers.substr(pos, lineEnd - pos);
                const std::size_t colon = line.find(':');
                if (colon != std::string::npos)
                {
                    std::string key = line.substr(0, colon);
                    std::transform(key.begin(), key.end(), key.begin(),
                                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                    std::string wanted = name;
                    std::transform(wanted.begin(), wanted.end(), wanted.begin(),
                                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                    if (key == wanted)
                    {
                        const std::string value = line.substr(colon + 1);
                        const std::size_t first = value.find_first_not_of(" \t");
                        const std::size_t last = value.find_last_not_of(" \t");
                        if (first == std::string::npos) return {};
                        return value.substr(first, last - first + 1);
                    }
                }
                if (end == std::string::npos) break;
                pos = end + 2;
            }
            return {};
        }

        bool writeAll(asio::ip::tcp::socket& socket, const char* data, std::size_t size)
        {
            std::size_t sent = 0;
            while (sent < size)
            {
                asio::error_code ec;
                const std::size_t written = asio::write(socket, asio::buffer(data + sent, size - sent), ec);
                if (ec || written == 0) return false;
                sent += written;
            }
            return true;
        }

        bool sendTextFrame(asio::ip::tcp::socket& socket, const std::string& payload)
        {
            std::string frame;
            frame.push_back(static_cast<char>(0x81));
            const std::size_t size = payload.size();
            if (size <= 125)
            {
                frame.push_back(static_cast<char>(size));
            }
            else if (size <= 65535)
            {
                frame.push_back(static_cast<char>(126));
                frame.push_back(static_cast<char>((size >> 8) & 0xFF));
                frame.push_back(static_cast<char>(size & 0xFF));
            }
            else
            {
                frame.push_back(static_cast<char>(127));
                for (int shift = 56; shift >= 0; shift -= 8)
                {
                    frame.push_back(static_cast<char>((size >> shift) & 0xFF));
                }
            }
            frame.append(payload);
            return writeAll(socket, frame.data(), frame.size());
        }
    }

    class ProgressHub::Session : public std::enable_shared_from_this<Session>
    {
    public:
        explicit Session(std::shared_ptr<asio::ip::tcp::socket> socket)
            : socket_(std::move(socket))
        {
        }

        bool send(const std::string& payload)
        {
            std::lock_guard<std::mutex> guard(writeMutex_);
            if (!socket_ || !socket_->is_open()) return false;
            return sendTextFrame(*socket_, payload);
        }

        void close()
        {
            std::lock_guard<std::mutex> guard(writeMutex_);
            if (socket_ && socket_->is_open())
            {
                asio::error_code ec;
                socket_->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
                socket_->close(ec);
            }
        }

    private:
        std::shared_ptr<asio::ip::tcp::socket> socket_;
        std::mutex writeMutex_;
    };

    ProgressHub::ProgressHub(std::string token) : token_(std::move(token))
    {
    }

    ProgressHub::~ProgressHub()
    {
        stop();
    }

    std::uint16_t ProgressHub::start(const std::string& host, std::uint16_t port)
    {
        const auto address = asio::ip::make_address(host);
        acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(
            io_, asio::ip::tcp::endpoint(address, port));
        port_ = static_cast<std::uint16_t>(acceptor_->local_endpoint().port());
        running_.store(true);
        thread_ = std::thread([this] { acceptLoop(); });
        return port_;
    }

    void ProgressHub::stop()
    {
        if (!running_.exchange(false)) return;

        asio::error_code ec;
        asio::io_context wakeIo;
        asio::ip::tcp::socket wake(wakeIo);
        wake.connect(asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1"), port_), ec);
        wake.close(ec);

        if (acceptor_) acceptor_->close(ec);
        io_.stop();

        {
            std::lock_guard<std::mutex> guard(mutex_);
            for (const auto& weak : sessions_)
            {
                if (auto session = weak.lock()) session->close();
            }
        }
        if (thread_.joinable()) thread_.join();
        for (auto& worker : workers_)
        {
            if (worker.joinable()) worker.join();
        }
    }

    void ProgressHub::acceptLoop()
    {
        while (running_.load())
        {
            auto socket = std::make_shared<asio::ip::tcp::socket>(io_);
            asio::error_code ec;
            acceptor_->accept(*socket, ec);
            if (ec)
            {
                if (!running_.load()) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
            if (!running_.load())
            {
                socket->close(ec);
                break;
            }
            auto worker = std::thread([this, socket] { handle(socket); });
            std::lock_guard<std::mutex> guard(mutex_);
            workers_.push_back(std::move(worker));
        }
    }

    void ProgressHub::handle(std::shared_ptr<asio::ip::tcp::socket> socket)
    {
        std::string request;
        request.reserve(4096);
        std::array<char, 1> byte{};
        bool handshakeDone = false;

        for (int i = 0; i < 8192; ++i)
        {
            asio::error_code ec;
            const std::size_t read = asio::read(*socket, asio::buffer(byte), ec);
            if (ec || read == 0) return;
            request.push_back(byte[0]);
            if (request.size() >= 4 && request.compare(request.size() - 4, 4, "\r\n\r\n") == 0)
            {
                handshakeDone = true;
                break;
            }
        }
        if (!handshakeDone) return;

        const std::string key = headerValue(request, "sec-websocket-key");
        const std::string protocols = headerValue(request, "sec-websocket-protocol");
        bool authorized = false;
        std::size_t start = 0;
        while (start <= protocols.size())
        {
            const std::size_t comma = protocols.find(',', start);
            const std::size_t end = comma == std::string::npos ? protocols.size() : comma;
            std::string candidate = protocols.substr(start, end - start);
            const std::size_t first = candidate.find_first_not_of(" \t");
            const std::size_t last = candidate.find_last_not_of(" \t");
            if (first != std::string::npos)
            {
                candidate = candidate.substr(first, last - first + 1);
                if (candidate == token_) authorized = true;
            }
            if (comma == std::string::npos) break;
            start = comma + 1;
        }
        if (key.empty() || !authorized)
        {
            const std::string response =
                "HTTP/1.1 401 Unauthorized\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            writeAll(*socket, response.data(), response.size());
            return;
        }
        const std::string accept = acceptKey(key);
        if (accept.empty()) return;

        const std::string response =
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Protocol: " + token_ + "\r\n"
            "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
        if (!writeAll(*socket, response.data(), response.size())) return;

        auto session = std::make_shared<Session>(socket);
        {
            std::lock_guard<std::mutex> guard(mutex_);
            sessions_.erase(std::remove_if(sessions_.begin(), sessions_.end(),
                                           [](const std::weak_ptr<Session>& weak) { return weak.expired(); }),
                            sessions_.end());
            sessions_.push_back(session);
        }
        core::logger::write("info", "ws.connected", "", {{"port", port_}});

        const nlohmann::json hello = {
            {"event", "connected"},
            {"ts", core::logger::nowIso8601Utc()},
            {"version", 1}
        };
        session->send(hello.dump());

        while (running_.load())
        {
            std::array<unsigned char, 8> header{};
            asio::error_code ec;
            if (asio::read(*socket, asio::buffer(header.data(), 2), ec) != 2 || ec) break;
            const unsigned char opcode = header[0] & 0x0F;
            const bool masked = (header[1] & 0x80) != 0;
            std::uint64_t length = header[1] & 0x7F;
            if (length == 126)
            {
                if (asio::read(*socket, asio::buffer(header.data(), 2), ec) != 2 || ec) break;
                length = (static_cast<std::uint64_t>(header[0]) << 8) | header[1];
            }
            else if (length == 127)
            {
                if (asio::read(*socket, asio::buffer(header.data(), 8), ec) != 8 || ec) break;
                length = 0;
                for (int i = 0; i < 8; ++i) length = (length << 8) | header[i];
            }
            if (length > 1024 * 1024) break;

            std::array<unsigned char, 4> mask{};
            if (masked && (asio::read(*socket, asio::buffer(mask), ec) != 4 || ec)) break;
            std::string payload(length, '\0');
            if (length > 0 &&
                (asio::read(*socket, asio::buffer(payload.data(), length), ec) != length || ec)) break;
            if (masked)
            {
                for (std::size_t i = 0; i < payload.size(); ++i)
                {
                    payload[i] = static_cast<char>(static_cast<unsigned char>(payload[i]) ^ mask[i % 4]);
                }
            }
            if (opcode == 0x8) break;
            if (opcode == 0x9)
            {
                std::string frame;
                frame.push_back(static_cast<char>(0x8A));
                frame.push_back(static_cast<char>(payload.size() & 0x7F));
                frame.append(payload);
                std::lock_guard<std::mutex> guard(mutex_);
                writeAll(*socket, frame.data(), frame.size());
            }
        }
        session->close();
        core::logger::write("info", "ws.disconnected", "", {{"port", port_}});
    }

    void ProgressHub::broadcast(const nlohmann::json& event)
    {
        std::vector<std::shared_ptr<Session>> targets;
        {
            std::lock_guard<std::mutex> guard(mutex_);
            for (const auto& weak : sessions_)
            {
                if (auto session = weak.lock()) targets.push_back(session);
            }
        }
        const std::string payload = event.dump();
        for (const auto& session : targets)
        {
            session->send(payload);
        }
    }
}
