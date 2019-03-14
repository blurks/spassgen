# Maintainer: Your Name <youremail@domain.com>
pkgname=spassgen
pkgver=0.1
pkgrel=1
epoch=
pkgdesc="A simple password generator."
arch=(any)
url=""
license=('unknown')
groups=()
depends=()
makedepends=()
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=
changelog=
source=("$pkgname-$pkgver.tar.gz")
noextract=()
md5sums=('0cad9638f956f8a7cb6b73528114febc')
validpgpkeys=()

build() {
	cd "$pkgname-$pkgver"
	make
}

package() {
	cd "$pkgname-$pkgver"
	make DESTDIR="$pkgdir/" install
}
