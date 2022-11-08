#pragma once

#include <cstdio>

// ページディレクトリの個数
const size_t kPageDirectoryCount = 64;

// ページテーブルの設定
//      仮想アドレス = 物理アドレスとなるようにページテーブルを設定する
void SetupIdentityPageTable();

void InitializePaging();