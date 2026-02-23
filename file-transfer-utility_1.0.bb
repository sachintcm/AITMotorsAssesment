SUMMARY = "TCP-based file transfer utility with SHA256 integrity verification"
DESCRIPTION = "A cross-platform file transfer utility that enables secure, large-file transfers \
(up to 16GB) between networked computers with SHA256 integrity verification. \
Supports client-server architecture over TCP."

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=d41d8cd98f00b204e9800998ecf8427e"  # Placeholder - update with actual license

SRC_URI = "git://github.com/sachintcm/AITMotorsAssesment.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"

DEPENDS = "openssl"

S = "${WORKDIR}/git"

do_compile() {
    oe_runmake
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/bin/client ${D}${bindir}/file-transfer-client
    install -m 0755 ${B}/bin/server ${D}${bindir}/file-transfer-server
}

FILES_${PN} = "${bindir}/file-transfer-client ${bindir}/file-transfer-server"

# Optional: Add runtime dependencies if needed
RDEPENDS_${PN} = "openssl"