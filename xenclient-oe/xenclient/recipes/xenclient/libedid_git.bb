DESCRIPTION = "libedid"
LICENSE = "LGPLv2.1"
LIC_FILES_CHKSUM = "file://COPYING;md5=321bf41f280cf805086dd5a720b37785"
DEPENDS = ""

SRC_URI = "lndir://libedid"

S = "${WORKDIR}/git"

inherit autotools
inherit pkgconfig
inherit xenclient

FILES_${PN}-dev += "/usr/bin/libedid-config"

PR = "r1"

