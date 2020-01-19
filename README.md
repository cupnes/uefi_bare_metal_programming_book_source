# 『フルスクラッチで作る!UEFIベアメタルプログラミング』 SOURCE FILES

This is a [Re:VIEW](https://reviewml.org/) format source files for my book 『フルスクラッチで作る!UEFIベアメタルプログラミング』.

PDF and HTML versions of this book are available at http://yuma.ohgami.jp .

## Setup

Please use the following docker container.

 * [yohgami/review - Docker Hub](https://hub.docker.com/r/yohgami/review)

### Docker Pull Command

```sh
$ docker pull yohgami/review
```

## How to Generate a PDF File or HTML Files

### PDF

```sh
$ ./build-in-docker.sh pdf
```

Generated as follows:

```sh
$ ls articles/*.pdf
articles/UEFI-Bare-Metal-Programming.pdf
```

### HTML

```sh
$ ./build-in-docker.sh html
```

Generated as follows:

```sh
$ ls articles/html
appendix.html  images  index.html  intro.html  main_1.html  main_2.html  main_3.html  main_4.html  main_5.html  postscript.html  refs.html  style.css
```

## Reference

The the Re:VIEW style files and the Docker container used the following repository content:

 * [C89-FirstStepReVIEW-v2](https://github.com/TechBooster/C89-FirstStepReVIEW-v2)
