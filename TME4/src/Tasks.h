#ifndef TASKS_H
#define TASKS_H

#include <QImage>
#include <filesystem>
#include <qimage.h>
#include "BoundedBlockingQueue.h"

namespace pr {

using FileQueue = BoundedBlockingQueue<std::filesystem::path>;

const std::filesystem::path FILE_POISON{};

// load/resize/save 
void treatImage(FileQueue& fileQueue, const std::filesystem::path& outputFolder);

struct TaskData {
    std::filesystem::path filePath;
    std::unique_ptr<QImage> image;

    TaskData(const std::filesystem::path& path, std::unique_ptr<QImage> img)
        : filePath(path), image(std::move(img)) {}
};

using ImageTaskQueue = BoundedBlockingQueue<TaskData*>;

// On suppose (attention à la sécurité...) que le poison est nullptr
constexpr TaskData* TASK_POISON = nullptr;

//using ImageTaskQueue = BoundedBlockingQueue<TaskData>;

// TODO
void reader(FileQueue& fileQueue, ImageTaskQueue& imageQueue);
void resizer(ImageTaskQueue& imageQueue, ImageTaskQueue& resizedQueue);
void saver(ImageTaskQueue& resizedQueue, const std::filesystem::path& outputFolder);

} // namespace pr

#endif // TASKS_H