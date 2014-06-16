do_apply_patchqueue(){
    # Handle insane patchqueue
    set -e
    GUILT=guilt
    MACHINE="${MACHINE_ARCH}"

    export EMAIL="local.changes@example.com"
    export GIT_AUTHOR_NAME="Local Changes"

    if [ ! -d ${WORKDIR}/git ] && [ -d ${WORKDIR}/patchqueue/.git ]; then
        # We are not using git for the upstream kernel (tarball from kernel.org)
	# and patch queue is from git repo.
	pushd ${S}
        if [ ! -d ${S}/.git ]; then
            git init
            git add -f .
            git commit -m "import from upstream"
            ln -sf ${WORKDIR}/patchqueue .git/patches
        fi
    elif [ ! -d ${WORKDIR}.git ]; then
	GUILT=quilt
	pushd ${S}
        if [ -f ${WORKDIR}/patchqueue/master/series ]; then
    	   ln -sf ../patchqueue/master ${S}/patches
                QUILT_PATCHES=${WORKDIR}/patchqueue/master
            else
    	        ln -sf ../patchqueue ${$}/patches
                QUILT_PATCHES=${WORKDIR}/patchqueue
            fi
	    touch ${QUILT_PATCHES}/guards
    elif [ ! -d ${WORKDIR}/git/.git ]; then
	pushd ${WORKDIR}/git
	GUILT=quilt
        if [ -d ${WORKDIR}/patchqueue ]; then
	    if [ -f ${WORKDIR}/patchqueue/master/series ]; then
    	        ln -sf ../patchqueue/master ${WORKDIR}/git/patches
                QUILT_PATCHES=${WORKDIR}/patchqueue/master
            else
    	        ln -sf ../patchqueue ${WORKDIR}/git/patches
                QUILT_PATCHES=${WORKDIR}/patchqueue
            fi
	    touch ${QUILT_PATCHES}/guards
        else
            # If there is no .git directory then we are not using the
            # repository directly (but rather a copy generated from a tarball)
            # This is the normally the case for builds external to Citrix.
            # In this case the patchqueue has been unpacked into the
            # same directory as the sources.
            # TODO: Fix this so the patchqueue gets installed
            # in the correct location.
    	    QUILT_PATCHES=${WORKDIR}/git/master
       fi
    else
        # If patches is a link then we have set up the patchqueue symlink already
	if [ ! -L ${WORKDIR}/git/.git/patches ]; then
            if ! git symbolic-ref HEAD 2>/dev/null ; then
                # If we're not on a branch, create one for guilt.
                git checkout -b make-guilt-happy
            fi
            [ -d ${WORKDIR}/git/.git/patches ] && rmdir ${WORKDIR}/git/.git/patches
            branch=$(cd ${WORKDIR}/git && git branch | grep '*' | cut -d' ' -f2)
            [ "$branch" != "master" ] && (ln -s master ${WORKDIR}/patchqueue/$branch)
            ln -sf ${WORKDIR}/patchqueue ${WORKDIR}/git/.git/patches
	fi
	pushd ${WORKDIR}/git
    fi
    echo "patch queue in ${QUILT_PATCHES}"
    # Create series and status if missing, push pq, pop folder
    if [ -d .git/patches/master ]; then
	pushd .git/patches/master
	if [ ! -e series ]; then
	    cp series.common series
	    [ -e series.$MACHINE ] && cat series.$MACHINE >> series
	fi
	[ ! -f status ] && touch status
	popd
    fi
    $GUILT push -a
    popd
}
addtask apply_patchqueue after do_patch before do_configure
