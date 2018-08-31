
add_library(PTHREAD INTERFACE)

target_include_directories(PTHREAD INTERFACE ${ZEPHYR_BASE}/include/posix)

target_link_libraries(PTHREAD INTERFACE zephyr_interface)

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            pthread_common.c
            pthread_sched.c
            clock.c
            sleep.c
            timer.c
            IFDEF:${CONFIG_PTHREAD_IPC} pthread_cond.c
                                        pthread_mutex.c
                                        pthread_barrier.c
                                        pthread.c
                                        pthread_rwlock.c
                                        semaphore.c
                                        pthread_key.c
            IFDEF:${CONFIG_POSIX_MQUEUE} mqueue.c
            IFDEF:${CONFIG_POSIX_FS} fs.c
)

add_library(posix STATIC ${PRIVATE_SOURCES})
target_link_libraries(posix PUBLIC zephyr_interface PTHREAD)
target_link_libraries(zephyr_app_linking INTERFACE posix)
