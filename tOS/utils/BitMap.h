//
// Created by xinyang on 2020/6/2.
//

#ifndef TOS_BITMAP_H
#define TOS_BITMAP_H

#include "../tOS_config.h"
#include <cstdint>
#include <type_traits>

namespace tOS {
    /***** bitmap大小类型定义 *****/
    using BITMAP_SIZE_8 = std::uint8_t;
    using BITMAP_SIZE_16 = std::uint16_t;
    using BITMAP_SIZE_32 = std::uint32_t;
    using BITMAP_SIZE_64 = std::uint64_t;

    /* bitmap类定义
     * BITMAP_SIZE_T: bitmap大小类型
     */
    template<class BITMAP_SIZE_T>
    class BitMap {
    private:
        BITMAP_SIZE_T bit;
    public:
        constexpr static std::size_t size = std::is_same_v<BITMAP_SIZE_T, BITMAP_SIZE_8> ? 8 :
                                            std::is_same_v<BITMAP_SIZE_T, BITMAP_SIZE_16> ? 16 :
                                            std::is_same_v<BITMAP_SIZE_T, BITMAP_SIZE_32> ? 32 :
                                            std::is_same_v<BITMAP_SIZE_T, BITMAP_SIZE_64> ? 64 : 0;
        static_assert(size != 0);

        BitMap(BITMAP_SIZE_T b = 0) : bit(b) {};

        inline bool get_bit(int i) const {
            return (bit >> i) & 1;
        }

        inline void set_bit(int i) {
            bit |= static_cast<BITMAP_SIZE_T>(1) << i;
        }

        inline void clear_bit(int i) {
            bit &= ~(static_cast<BITMAP_SIZE_T>(1) << i);
        }

        inline operator BITMAP_SIZE_T() const {
            return bit;
        }

        // 取最低位1的bit位置，折半查找，4bit内查表。
        inline std::size_t lowbit() const {
            static std::size_t lowbit_map[] = {
                    -1u, 0, 1, 0, 2, 0, 1, 0,
                    3, 0, 1, 0, 2, 0, 1, 0,
            };
            if (bit == 0) return -1;
            BITMAP_SIZE_T v = bit;
            std::size_t n = 0;
            if constexpr (size >= 64) {
                if (v & 0xffffffff) { v &= 0xffffffff; }
                else { v >>= 32, n += 32; }
            }
            if constexpr (size >= 32) {
                if (v & 0xffff) { v &= 0xffff; }
                else { v >>= 16, n += 16; }
            }
            if constexpr (size >= 16) {
                if (v & 0xff) { v &= 0xff; }
                else { v >>= 8, n += 8; }
            }
            if constexpr (size >= 8) {
                if (v & 0xf) { v &= 0xf; }
                else { v >>= 4, n += 4; }
            }
            return n + lowbit_map[v];
        }

        // 计算为1的bit的个数，分组归并算法。
        inline std::size_t bitcnt() const {
            std::size_t n = bit - ((bit >> 1) & (BITMAP_SIZE_T) 0x5555555555555555);
            n = (n & (BITMAP_SIZE_T) 0x3333333333333333) + ((n >> 2) & (BITMAP_SIZE_T) 0x3333333333333333);
            return (((n + (n >> 4)) & (BITMAP_SIZE_T) 0x0F0F0F0F0F0F0F0F) * (BITMAP_SIZE_T) 0x0101010101010101)
                    >> (sizeof(BITMAP_SIZE_T) * 8 - 8);
        }
    };
}

#endif /* TOS_BITMAP_H */
