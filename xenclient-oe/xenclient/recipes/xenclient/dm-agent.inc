DESCRIPTION = "Daemon which spawns Device Models in a domain"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=4641e94ec96f98fabc56ff9cc48be14b"
DEPENDS = "xen xen-tools libv4v libdmbus dbus libevent libxcxenstore"

SRC_URI = "lndir://dm-agent"
S = "${WORKDIR}/git"

EXTRA_OEMAKE += "LIBDIR=${STAGING_LIBDIR}"

inherit autotools
inherit xenclient
