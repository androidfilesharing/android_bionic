/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef BIONIC_ATOMIC_ARM_H
#define BIONIC_ATOMIC_ARM_H

__ATOMIC_INLINE__ void __bionic_memory_barrier() {
#if defined(ANDROID_SMP) && ANDROID_SMP == 1
  __asm__ __volatile__ ( "dmb ish" : : : "memory" );
#else
  /* A simple compiler barrier. */
  __asm__ __volatile__ ( "" : : : "memory" );
#endif

/* Define a full memory barrier, this is only needed if we build the
 * platform for a multi-core device. For the record, using a 'dmb'
 * instruction on a Nexus One device can take up to 180 ns even if
 * it is completely un-necessary on this device.
 *
 * NOTE: This is where the platform and NDK headers atomic headers are
 *        going to diverge. With the NDK, we don't know if the generated
 *        code is going to run on a single or multi-core device, so we
 *        need to be cautious.
 *
 *        I.e. on single-core devices, the helper immediately returns,
 *        on multi-core devices, it uses "dmb" or any other means to
 *        perform a full-memory barrier.
 *
 * There are three cases to consider for the platform:
 *
 *    - multi-core ARMv7-A       => use the 'dmb' hardware instruction
 *    - multi-core ARMv6         => use the coprocessor
 *    - single core ARMv6+       => do not use any hardware barrier
 */
#if defined(ANDROID_SMP) && ANDROID_SMP == 1

/* For ARMv7-A, we can use the 'dmb' instruction directly */
__ATOMIC_INLINE__ void __bionic_memory_barrier(void) {
    /* Note: we always build in ARM or Thumb-2 on ARMv7-A, so don't
     * bother with __ATOMIC_SWITCH_TO_ARM */
    __asm__ __volatile__ ( "dmb" : : : "memory" );
}

#else /* !ANDROID_SMP */

__ATOMIC_INLINE__ void __bionic_memory_barrier(void) {
    /* A simple compiler barrier */
    __asm__ __volatile__ ( "" : : : "memory" );
}

#endif /* !ANDROID_SMP */

/* Compare-and-swap, without any explicit barriers. Note that this functions
 * returns 0 on success, and 1 on failure. The opposite convention is typically
 * used on other platforms.
 */
__ATOMIC_INLINE__ int __bionic_cmpxchg(int32_t old_value, int32_t new_value, volatile int32_t* ptr) {
  int32_t prev, status;
  do {
    __asm__ __volatile__ (
          "ldrex %0, [%3]\n"
          "mov %1, #0\n"
          "teq %0, %4\n"
#ifdef __thumb2__
          "it eq\n"
#endif
          "strexeq %1, %5, [%3]"
          : "=&r" (prev), "=&r" (status), "+m"(*ptr)
          : "r" (ptr), "Ir" (old_value), "r" (new_value)
          : "cc");
  } while (__builtin_expect(status != 0, 0));
  return prev != old_value;
}

/* Swap, without any explicit barriers. */
__ATOMIC_INLINE__ int32_t __bionic_swap(int32_t new_value, volatile int32_t* ptr) {
  int32_t prev, status;
  do {
    __asm__ __volatile__ (
          "ldrex %0, [%3]\n"
          "strex %1, %4, [%3]"
          : "=&r" (prev), "=&r" (status), "+m" (*ptr)
          : "r" (ptr), "r" (new_value)
          : "cc");
  } while (__builtin_expect(status != 0, 0));
  return prev;
}

/* Atomic decrement, without explicit barriers. */
__ATOMIC_INLINE__ int32_t __bionic_atomic_dec(volatile int32_t* ptr) {
  int32_t prev, tmp, status;
  do {
    __asm__ __volatile__ (
          "ldrex %0, [%4]\n"
          "sub %1, %0, #1\n"
          "strex %2, %1, [%4]"
          : "=&r" (prev), "=&r" (tmp), "=&r" (status), "+m"(*ptr)
          : "r" (ptr)
          : "cc");
  } while (__builtin_expect(status != 0, 0));
  return prev;
}

#endif /* SYS_ATOMICS_ARM_H */
