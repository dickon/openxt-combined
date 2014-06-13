PR = "r0"
PV = "2.2.5+git${SRCPV}"

include selinux_git.inc
include ${BPN}.inc

LIC_FILES_CHKSUM = "file://COPYING;md5=393a5ca445f6965873eca0259a17f833"

SRC_URI += "\
	file://policycoreutils-revert-run_init-open_init_pty.patch \
	file://policycoreutils-fix-sepolicy-install-path.patch \
	file://policycoreutils-make-O_CLOEXEC-optional.patch \
	"
