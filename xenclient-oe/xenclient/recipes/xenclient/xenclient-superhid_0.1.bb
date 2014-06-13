inherit module-compat
inherit xenclient

DESCRIPTION = "Superhid kernel module"
LICENSE = "LGPLv2.1"
LIC_FILES_CHKSUM = "file://../COPYING;md5=321bf41f280cf805086dd5a720b37785"

SRC_URI = "lndir://input"

S = "${WORKDIR}/git/superhid"

MAKE_TARGETS += "modules"
