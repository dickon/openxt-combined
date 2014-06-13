DESCRIPTION = "FUSE filesystem over icbinn service"
LICENSE = "LGPLv2.1"
LIC_FILES_CHKSUM = "file://COPYING;md5=321bf41f280cf805086dd5a720b37785"

SRC_URI = "lndir://icbinn.git"

DEPENDS = "libicbinn fuse"

S = "${WORKDIR}/git/icbinn-fuse"

inherit autotools
inherit pkgconfig
inherit xenclient

