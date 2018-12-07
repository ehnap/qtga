# QTga
## 简介
Qt TGA 格式插件改良，基于Qt5.11
## 支持类型
* 空数据图像
* 未压缩的颜色表图像
* 未压缩的真彩色图像
* 未压缩的黑白图像
* RLE压缩的颜色表图像
* RLE压缩的真彩色图像
* RLE压缩的黑白图像
> 即 ImageType为0、1、2、3、9、10、11的TGA图片
## 备注
* 对于TGA 2.0可以正确解析图像，暂时不支持取2.0扩展域信息(主要是Qt插件的扩展性问题，取出来找不到地方返回)
* 由于统一返回ARGB32的QImage，所以使用16位数据存储的TGA，解析之后会将颜色映射到32位上，将RGB分别置于高位得到32位颜色
> e.g.  GGGBBBBB ARRRRRGG -> AAAAAAARRRRR000GGGGG000BBBBB000
