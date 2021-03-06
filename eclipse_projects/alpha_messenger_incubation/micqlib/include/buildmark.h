/* $Id: buildmark.h,v 1.28 2005/02/20 15:31:16 kuhlmann Exp $
 *
 * mICQ version functions and ICQ client identifiers.
 */

#ifndef MICQ_BUILDMARK_H
#define MICQ_BUILDMARK_H

#define MICQ_VERSION VERSION

#define BUILD_MICQ     0xffffff42L
#define BUILD_MICQ_OLD 0x7d0001eaL

#define BUILD_PLATFORM_OTHER 0x01000000
#define BUILD_PLATFORM_WIN   0x02000000
#define BUILD_PLATFORM_BSD   0x03000000
#define BUILD_PLATFORM_LINUX 0x04000000
#define BUILD_PLATFORM_UNIX  0x05000000

#define BUILD_PLATFORM_AMIGAOS BUILD_PLATFORM_OTHER | 0x000001
#define BUILD_PLATFORM_BEOS    BUILD_PLATFORM_OTHER | 0x000002
#define BUILD_PLATFORM_QNX     BUILD_PLATFORM_OTHER | 0x000004
#define BUILD_PLATFORM_CYGWIN  BUILD_PLATFORM_WIN   | 0x000010
#define BUILD_PLATFORM_WINDOWS BUILD_PLATFORM_WIN   | 0x000020
#define BUILD_PLATFORM_OPENBSD BUILD_PLATFORM_BSD   | 0x000100
#define BUILD_PLATFORM_NETBSD  BUILD_PLATFORM_BSD   | 0x000200
#define BUILD_PLATFORM_FREEBSD BUILD_PLATFORM_BSD   | 0x000400
#define BUILD_PLATFORM_MACOSX  BUILD_PLATFORM_BSD   | 0x000800
#define BUILD_PLATFORM_AIX     BUILD_PLATFORM_UNIX  | 0x001000
#define BUILD_PLATFORM_HPUX    BUILD_PLATFORM_UNIX  | 0x002000
#define BUILD_PLATFORM_SOLARIS BUILD_PLATFORM_UNIX  | 0x004000
#define BUILD_PLATFORM_DEBIAN  BUILD_PLATFORM_LINUX | 0x010000

const        char  *BuildVersion (void);
const        char  *BuildAttribution (void);
extern const UDWORD BuildVersionNum;
extern const UDWORD BuildPlatformID;
extern const char  *BuildVersionText;

#endif /* MICQ_BUILDMARK_H */
