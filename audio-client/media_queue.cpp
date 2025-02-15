#include "media_queue.h"


void MediaQueue::Push(MediaData data) {
    std::lock_guard<std::mutex> lock(mutex);
    queue.push(std::move(data));
    cv.notify_one();
}

MediaData MediaQueue::Pop() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [this] { return !queue.empty(); });
    MediaData data = std::move(queue.front());
    queue.pop();
    return data;
}
