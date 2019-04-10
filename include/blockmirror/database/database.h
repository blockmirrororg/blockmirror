#pragma once

namespace blockmirror {
namespace database {

void initDb(); // 初始化
void saveDb(); // 另存为
void closeDb(); // 退出时关闭

}  // namespace database
}  // namespace blockmirror