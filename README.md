# ARLinux Steamworks

Standalone native ARM64 Steam RPC bridge and x86, x64 and ARM64EC Wine frontends. Sources, upstream revisions and license notices are retained. Local fixes live in Git, not a build-time patch set. This does not modify or package Steam Proton. `build.sh` consumes matching Wine headers, Unix build, host tools and LLVM-MinGW and produces the standalone payload. Python/Linux and Android integration remain in their respective products.

Build procedures: https://github.com/taowen/wine/blob/arlinux/arlinux/BUILD.md
