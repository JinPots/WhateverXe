// Copyright (c) 2026 b00t0x. BSD-3-Clause.
#pragma once
#include <string.h>

namespace XeDataPolicy {
enum class Client { None, Compiler, WebKitGPU };
// Identity comes from the running process's kernel code-signing state, never
// Info.plist, a basename, or caller-provided metadata. Safari updates and moves
// must not require enumerating executable locations.
inline bool webKitGPU(const char *identity, bool platformBinary, bool valid) {
    return platformBinary && valid && identity && !strcmp(identity, "com.apple.WebKit.GPU");
}
inline bool mediaGPU(const char *identity, bool platformBinary, bool valid) {
    if (!platformBinary || !valid || !identity) return false;
    if (!strcmp(identity, "com.apple.Music") ||
        !strcmp(identity, "com.apple.TV") ||
        !strcmp(identity, "com.apple.Podcasts") ||
        !strcmp(identity, "com.apple.controlcenter") ||
        !strcmp(identity, "com.apple.QuickTimePlayerX"))
        return true;
    return (!strncmp(identity, "com.apple.coremedia.audioencoder", 32) &&
            (identity[32] == '\0' || identity[32] == '.')) ||
           (!strncmp(identity, "com.apple.coremedia.audiodecoder", 32) &&
            (identity[32] == '\0' || identity[32] == '.')) ||
           (!strncmp(identity, "com.apple.coremedia.videoencoder", 32) &&
            (identity[32] == '\0' || identity[32] == '.')) ||
           (!strncmp(identity, "com.apple.coremedia.videodecoder", 32) &&
            (identity[32] == '\0' || identity[32] == '.'));
}
inline bool service(const char *path, Client client = Client::Compiler) {
    // Compiler access keeps its original exact-path contract. WebKit has no
    // path-based fallback: even a familiar location must pass signature checks.
    return client == Client::Compiler && path && !strcmp(path,
        "/System/Library/Frameworks/Metal.framework/Versions/A/"
        "XPCServices/MTLCompilerService.xpc/Contents/MacOS/MTLCompilerService");
}
inline bool file(const char *path, bool executable) {
    if (!path) return false;
    // vn_getpath may expose the Data mount rather than its firmlink.
    constexpr const char data[] = "/System/Volumes/Data";
    if (!strncmp(path, data, sizeof(data)-1)) path += sizeof(data)-1;
    if (!executable && (!strcmp(path, "/Library/GPUBundles/XeCompilerAdapter.bundle") ||
        !strcmp(path, "/Library/GPUBundles/XeCompilerAdapter.bundle/Contents") ||
        !strcmp(path, "/Library/GPUBundles/XeCompilerAdapter.bundle/Contents/MacOS"))) return true;
    constexpr const char dir[] = "/Library/GPUBundles/XeCompilerAdapter.bundle/Contents/";
    if (strncmp(path, dir, sizeof(dir)-1)) return false;
    path += sizeof(dir)-1;
    if (!strcmp(path, "MacOS/adapter.dylib") || !strcmp(path, "MacOS/libigc-tgl.dylib")) return true;
    return !executable && (!strcmp(path, "MacOS/runtime.igc64.bc") || !strcmp(path, "Info.plist"));
}
inline Client clientForFile(const char *path, bool executable) {
    if (file(path, executable)) return Client::Compiler;
    // WebKit already permits Data reads; allow executable maps of fixed GPU images.
    if (!path || !executable) return Client::None;
    constexpr const char data[] = "/System/Volumes/Data";
    if (!strncmp(path, data, sizeof(data)-1)) path += sizeof(data)-1;
    constexpr const char va[] = "/Library/GPUBundles/AppleIntelTGLGraphicsVADriver.bundle/Contents/MacOS/";
    if (!strncmp(path, va, sizeof(va)-1)) {
        path += sizeof(va)-1;
        return !strcmp(path, "AppleIntelTGLGraphicsVADriver") || !strcmp(path, "TGLVA") ?
            Client::WebKitGPU : Client::None;
    }
    constexpr const char metal[] = "/Library/GPUBundles/AppleIntelTGLGraphicsMTLDriver.bundle/Contents/MacOS/";
    if (!strncmp(path, metal, sizeof(metal)-1)) {
        path += sizeof(metal)-1;
        return (!strcmp(path, "AppleIntelTGLGraphicsMTLDriver") || !strcmp(path, "descriptor.dylib") ||
            !strcmp(path, "impostor.dylib") || !strcmp(path, "libigdmd.dylib")) ?
            Client::WebKitGPU : Client::None;
    }
    constexpr const char tglVAME[] = "/Library/GPUBundles/AppleIntelTGLGraphicsVAME.bundle/Contents/MacOS/";
    constexpr const char iclVAME[] = "/Library/GPUBundles/AppleIntelICLGraphicsVAME.bundle/Contents/MacOS/";
    if (!strncmp(path, tglVAME, sizeof(tglVAME)-1)) {
        path += sizeof(tglVAME)-1;
    } else if (!strncmp(path, iclVAME, sizeof(iclVAME)-1)) {
        path += sizeof(iclVAME)-1;
    } else {
        return Client::None;
    }
    return !strcmp(path, "AppleIntelTGLGraphicsVAME") ||
        !strcmp(path, "AppleIntelICLGraphicsVAME") ? Client::WebKitGPU : Client::None;
}
inline bool readOnly(int mode) { return mode == 1; } // FREAD, no create/truncate/write flags.
inline bool executeOnly(int prot, int flags) {
    return (prot == 4 || prot == 5) && (flags & 3) == 2; // RX/X, MAP_PRIVATE.
}
}
