#ifndef CRITICAL_SECTION_MANAGER_H_
#define CRITICAL_SECTION_MANAGER_H_

/*
 * The imported stack expects global critical section hooks.
 * This compatibility layer keeps them as no-ops by default.
 */
#define CriticalSection_Lock() do { } while (0)
#define CriticalSection_Unlock() do { } while (0)

#endif /* CRITICAL_SECTION_MANAGER_H_ */
