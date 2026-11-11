#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <chrono>
#include <cstddef>

// Потокобезопасный буфер для int16_t отсчётов (I/Q interleaved: I0,Q0,I1,Q1,...)
// Один поток пишет (push), другой читает (pop) с таймаутом.
class RingBuffer
{
public:
    explicit RingBuffer(size_t max_samples = 1024 * 1024)
        : _max_samples(max_samples)
    {
    }

    void push(const int16_t *data, size_t count)
    {
        std::unique_lock<std::mutex> lock(_mtx);
        for (size_t i = 0; i < count; i++)
        {
            _buffer.push_back(data[i]);
        }
        // Защита от неограниченного роста, если читающая сторона отстаёт
        while (_buffer.size() > _max_samples)
        {
            _buffer.pop_front();
        }
        lock.unlock();
        _cv.notify_all();
    }

    // Пытается прочитать ровно count элементов (int16_t значений, НЕ комплексных отсчётов)
    // Возвращает true если удалось полностью заполнить out в пределах timeout_ms.
    bool pop(int16_t *out, size_t count, int timeout_ms)
    {
        std::unique_lock<std::mutex> lock(_mtx);

        auto pred = [this, count]() { return _buffer.size() >= count || _shutdown; };

        if (timeout_ms < 0)
        {
            _cv.wait(lock, pred);
        }
        else
        {
            if (!_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), pred))
            {
                return false; // timeout
            }
        }

        if (_shutdown || _buffer.size() < count)
        {
            return false;
        }

        for (size_t i = 0; i < count; i++)
        {
            out[i] = _buffer.front();
            _buffer.pop_front();
        }
        return true;
    }

    size_t available() const
    {
        std::lock_guard<std::mutex> lock(_mtx);
        return _buffer.size();
    }

    void shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(_mtx);
            _shutdown = true;
        }
        _cv.notify_all();
    }

private:
    mutable std::mutex _mtx;
    std::condition_variable _cv;
    std::deque<int16_t> _buffer;
    size_t _max_samples;
    bool _shutdown = false;
};
