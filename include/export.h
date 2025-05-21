#ifndef STORAGE_INCLUDE_EXPORT_H_
#define STORAGE_INCLUDE_EXPORT_H_

#if !defined(DB_EXPORT)

#if defined(KIRISAMEDB_SHARED_LIBRARY)
#if defined(_WIN32)

#if defined(KIRISAMEDB_COMPILE_LIBRARY)
#define DB_EXPORT __declspec(dllexport)
#else
#define DB_EXPORT __declspec(dllimport)
#endif  // defined(KIRISAMEDB_COMPILE_LIBRARY)

#else  // defined(_WIN32)
#if defined(KIRISAMEDB_COMPILE_LIBRARY)
#define DB_EXPORT __attribute__((visibility("default")))
#else
#define DB_EXPORT
#endif
#endif  // defined(_WIN32)

#else  // defined(KIRISAMEDB_SHARED_LIBRARY)
#define DB_EXPORT
#endif

#endif  // !defined(DB_EXPORT)

#endif  // STORAGE_INCLUDE_EXPORT_H_
