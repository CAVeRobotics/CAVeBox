#ifndef QUEUE_H
#define QUEUE_H

#include <mutex>
#include <queue>

template<typename T>
class Queue
{
    public:
        const T& Front(void);
        const T& Back(void);
        bool Empty(void);
        std::size_t Size(void);
        void Push(const T &value);
        void Pop(void);

    private:
        std::queue<T> queue_;
        std::mutex mutex_;
};

template<typename T>
const T& Queue<T>::Front(void)
{
    std::lock_guard<std::mutex> lock(mutex_);

    return queue_.front();
}

template<typename T>
const T& Queue<T>::Back(void)
{
    std::lock_guard<std::mutex> lock(mutex_);

    return queue_.back();
}

template<typename T>
bool Queue<T>::Empty(void)
{
    std::lock_guard<std::mutex> lock(mutex_);

    return queue_.empty();
}

template<typename T>
std::size_t Queue<T>::Size(void)
{
    std::lock_guard<std::mutex> lock(mutex_);

    return queue_.size();
}

template<typename T>
void Queue<T>::Push(const T &value)
{
    std::lock_guard<std::mutex> lock(mutex_);

    queue_.push(value);
}

template<typename T>
void Queue<T>::Pop(void)
{
    std::lock_guard<std::mutex> lock(mutex_);

    queue_.pop();
}

#endif // QUEUE_H