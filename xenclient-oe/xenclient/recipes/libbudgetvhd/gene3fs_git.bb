DESCRIPTION = "gene3fs"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=4641e94ec96f98fabc56ff9cc48be14b"

DEPENDS += "libbudgetvhd"
DEPENDS += "e2fsprogs"

SRC_URI = "lndir://gene3fs.git"

S = "${WORKDIR}/git/gene3fs"

inherit autotools
inherit pkgconfig
inherit xenclient

BBCLASSEXTEND="native"
