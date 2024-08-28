# Maintainer: Your Name <youremail@example.com>
pkgname=task-heatmap
pkgver=1.0
pkgrel=1
pkgdesc="A simple task tracker with a GTK-based heatmap visualization"
arch=('x86_64')
url="https://yourprojecturl.example.com"
license=('MIT')
depends=('gtk3')
source=("task_heatmap.c" "Makefile")
md5sums=('SKIP' 'SKIP')

build() {
  cd "$srcdir"
  make
}

package() {
  cd "$srcdir"
  make DESTDIR="$pkgdir" PREFIX=/usr install
}


