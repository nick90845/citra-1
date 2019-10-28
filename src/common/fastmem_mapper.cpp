// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.


#include <cstdlib>
#include "common/fastmem_mapper.h"

namespace Common {

struct FastmemMapper::Impl {};

FastmemMapper::FastmemMapper(std::size_t shmem_required) : impl(std::make_unique<Impl>()) {}

FastmemMapper::~FastmemMapper() {}

u8* FastmemMapper::Allocate(std::size_t size) {
    return static_cast<u8*>(std::malloc(size));
}

u8* FastmemMapper::AllocRegion() {
    return nullptr;
}

void FastmemMapper::Map(Memory::PageTable&, VAddr vaddr, u8* backing_memory, std::size_t size) {}

void FastmemMapper::Unmap(Memory::PageTable&, VAddr vaddr, std::size_t size) {}

} // namespace Common
