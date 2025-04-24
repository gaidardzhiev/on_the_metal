#!/bin/sh
#the script builds binutils and gcc cross compiler for arm_v5

export TARGET=arm-none-eabi
export PREFIX=/opt/arm_gcc_binutils
export PATH=$PATH:$PREFIX/bin
export GETNUMCPUS=`grep -c '^processor' /proc/cpuinfo`
export JOBS='-j '$GETNUMCPUS''
export GCC=12.2.0
export BINUTILS=2.40
export DIR=/home/src/compilers/arm_gcc_binutils


fprep() {
	mkdir -p $DIR
	cd $DIR
w	get https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS.tar.gz
	wget https://ftp.gnu.org/gnu/gcc/gcc-$GCC/gcc-$GCC.tar.gz
	tar xfv binutils-$BINUTILS.tar.gz
	tar xfv gcc-$GCC.tar.gz
	rm *.tar.gz
	ln -s binutils-$BINUTILS binutils-patch
	patch -p0 < arm-patch && return 0 || return 2
}

fbinutils() {
	cd $DIR/binutils-$BINUTILS
	./configure \
		--targer=$TARGET \
		--prefix=$PREFIX
	echo "MAKEINFO = :" >> Makefile
	make $JOBS all
	make install && return 0 || return 3
}


fgcc() {
	cd $DIR/gcc-$GCC
	./configure \
		--target=$TARGET \
		--prefix=$PREFIX \
		--without-headers \
		--with-newlib \
		--with-gnu-as \
		--with-gnu-ld \
		--enable-languages='c' \
		--enable-frame-pointer=no
	make $JOBS all-gcc
	make install-gcc && return 0 || return 4
}

flibgcc() {
	cd $DIR/gcc-$GCC
	make $JOBS all-target-libgcc CFLAGS_FOR_TARGET="-g -02"
	make install-target-libgcc && return 0 || return 5
}

{ fprep && fbinutils && fgcc && flibgcc; RET=$?; } || exit 1

[ "$RET" -eq 0 ] 2>/dev/null || printf "%s\n" "$RET"
