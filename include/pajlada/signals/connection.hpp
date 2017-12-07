#pragma once

#include <functional>
#include <memory>

namespace pajlada {
namespace Signals {

namespace detail {

class CallbackBodyBase
{
protected:
    CallbackBodyBase() = delete;

    explicit CallbackBodyBase(uint64_t _index)
        : index(_index)
        , connected(true)
        , blocked(false)
        , subscriberRefCount(1)
    {
    }

public:
    const uint64_t index;

    virtual ~CallbackBodyBase() = default;

    void
    addRef()
    {
        ++this->subscriberRefCount;
    }

    bool
    disconnect()
    {
        --this->subscriberRefCount;

        if (this->subscriberRefCount > 0) {
            return false;
        }

        if (this->connected) {
            this->connected = false;

            return true;
        }

        return false;
    }

    bool
    isConnected() const
    {
        return this->connected;
    }

    bool
    block()
    {
        if (this->blocked) {
            return false;
        }

        this->blocked = true;

        return true;
    }

    bool
    unblock()
    {
        if (!this->blocked) {
            return false;
        }

        this->blocked = false;

        return true;
    }

    bool
    isBlocked() const
    {
        return this->blocked;
    }

private:
    bool connected;
    bool blocked;

    unsigned subscriberRefCount;
};

template <typename... Args>
class CallbackBody : public CallbackBodyBase
{
public:
    CallbackBody() = delete;

    explicit CallbackBody(uint64_t _index)
        : CallbackBodyBase(_index)
    {
    }

    virtual ~CallbackBody() = default;

    using FunctionSignature = std::function<void(Args...)>;

    FunctionSignature func;

    void
    invoke(Args... args)
    {
        this->func(args...);
    }
};

}  // namespace detail

class Connection
{
public:
    Connection()
    {
    }

    Connection(const Connection &other)
        : weakCallbackBody(other.weakCallbackBody)
    {
        std::shared_ptr<detail::CallbackBodyBase> connectionBody(this->weakCallbackBody.lock());
        if (!connectionBody) {
            return;
        }
        connectionBody->addRef();
    }

    Connection(const std::weak_ptr<detail::CallbackBodyBase> &connectionBody)
        : weakCallbackBody(connectionBody)
    {
        std::shared_ptr<detail::CallbackBodyBase> _connectionBody(this->weakCallbackBody.lock());
        if (!_connectionBody) {
            return;
        }
        _connectionBody->addRef();
    }

    Connection(Connection &&other)
        : weakCallbackBody(std::move(other.weakCallbackBody))
    {
        other.weakCallbackBody.reset();
    }

    Connection &
    operator=(Connection &&other)
    {
        if (&other == this) {
            return *this;
        }

        this->weakCallbackBody = std::move(other.weakCallbackBody);
        other.weakCallbackBody.reset();

        return *this;
    }

    Connection &
    operator=(const Connection &other)
    {
        if (&other == this) {
            return *this;
        }

        {
            std::shared_ptr<detail::CallbackBodyBase> connectionBody(this->weakCallbackBody.lock());
            if (connectionBody) {
                connectionBody->disconnect();
            }
        }

        this->weakCallbackBody = other.weakCallbackBody;

        {
            std::shared_ptr<detail::CallbackBodyBase> connectionBody(this->weakCallbackBody.lock());
            if (connectionBody) {
                connectionBody->addRef();
            }
        }

        return *this;
    }

    bool
    disconnect()
    {
        std::shared_ptr<detail::CallbackBodyBase> connectionBody(this->weakCallbackBody.lock());
        if (!connectionBody) {
            return false;
        }

        return connectionBody->disconnect();
    }

    bool
    isConnected()
    {
        std::shared_ptr<detail::CallbackBodyBase> connectionBody(this->weakCallbackBody.lock());
        if (!connectionBody) {
            return false;
        }

        return connectionBody->isConnected();
    }

    bool
    block()
    {
        std::shared_ptr<detail::CallbackBodyBase> connectionBody(this->weakCallbackBody.lock());
        if (!connectionBody) {
            return false;
        }

        return connectionBody->block();
    }

    bool
    unblock()
    {
        std::shared_ptr<detail::CallbackBodyBase> connectionBody(this->weakCallbackBody.lock());
        if (!connectionBody) {
            return false;
        }

        return connectionBody->unblock();
    }

    bool
    isBlocked()
    {
        std::shared_ptr<detail::CallbackBodyBase> connectionBody(this->weakCallbackBody.lock());
        if (!connectionBody) {
            return false;
        }

        return connectionBody->isBlocked();
    }

    template <typename... Args>
    void
    invoke(Args... args)
    {
        std::shared_ptr<detail::CallbackBodyBase> connectionBody(this->weakCallbackBody.lock());
        if (!connectionBody) {
            return;
        }

        try {
            auto advancedConnectionBody =
                std::static_pointer_cast<detail::CallbackBody<Args...>>(connectionBody);
            advancedConnectionBody->invoke(args...);
        } catch (...) {
            // TODO: Figure out which exceptino is thrown here
        }
    }

private:
    std::weak_ptr<detail::CallbackBodyBase> weakCallbackBody;
};

class ScopedConnection : public Connection
{
    ScopedConnection() = delete;

public:
    ScopedConnection(ScopedConnection &&other)
        : Connection(std::move(other))
    {
    }

    ScopedConnection(Connection &&other)
        : Connection(std::move(other))
    {
    }

    ScopedConnection(const Connection &other)
        : Connection(other)
    {
    }

    ScopedConnection(const ScopedConnection &other)
        : Connection(other)
    {
    }

    ~ScopedConnection()
    {
        this->disconnect();
    }
};

}  // namespace Signals
}  // namespace pajlada
