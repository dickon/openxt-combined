DESCRIPTION = "Power Management utility for XenClient"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=4641e94ec96f98fabc56ff9cc48be14b"
DEPENDS = "xenclient-idl dbus xen-tools libxcdbus libxenacpi xenclient-rpcgen-native pciutils"

SRC_URI = "lndir://xctools"

EXTRA_OECONF += "--with-idldir=${STAGING_IDLDIR}"

S = "${WORKDIR}/git/pmutil"

inherit autotools
inherit xenclient

