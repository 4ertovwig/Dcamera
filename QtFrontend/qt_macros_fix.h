/*
 * This file is part of Dcamera project.
 * See the LICENSE file in the root directory for full license terms.
 */
#pragma once

#if defined(USE_CLANG)
// turn off all conflicted macros(in compile with Clang)
#ifdef kill_dependency
#undef kill_dependency
#endif

#ifdef atomic_flag_test_and_set_explicit
#undef atomic_flag_test_and_set_explicit
#endif

#ifdef atomic_flag_clear_explicit
#undef atomic_flag_clear_explicit
#endif

#ifdef atomic_flag_test_and_set
#undef atomic_flag_test_and_set
#endif

#ifdef atomic_flag_clear
#undef atomic_flag_clear
#endif

#ifdef atomic_init
#undef atomic_init
#endif

#ifdef atomic_thread_fence
#undef atomic_thread_fence
#endif

#ifdef atomic_signal_fence
#undef atomic_signal_fence
#endif

#ifdef atomic_is_lock_free
#undef atomic_is_lock_free
#endif

#ifdef atomic_store
#undef atomic_store
#endif

#ifdef atomic_load
#undef atomic_load
#endif

#ifdef atomic_exchange
#undef atomic_exchange
#endif

#ifdef atomic_compare_exchange_strong
#undef atomic_compare_exchange_strong
#endif

#ifdef atomic_compare_exchange_weak
#undef atomic_compare_exchange_weak
#endif

#ifdef atomic_fetch_add
#undef atomic_fetch_add
#endif

#ifdef atomic_fetch_sub
#undef atomic_fetch_sub
#endif

#ifdef atomic_fetch_and
#undef atomic_fetch_and
#endif

#ifdef atomic_fetch_or
#undef atomic_fetch_or
#endif

#ifdef atomic_fetch_xor
#undef atomic_fetch_xor
#endif
#endif