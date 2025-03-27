# LVGL Demos Example

若屏幕分辨率为 128 * 128，巩膜图像的分辨率为 200 * 200（巩膜决定了瞳孔的颜色，中央区域应与所需的虹膜尺寸匹配），巩膜图像可以为空白。注意，巩膜大小大于屏幕分辨率才可以自由移动。

虹膜图像应该是展开的，默认虹膜直径为 80，对应展开图像大小为 256*64。

上眼睑和下眼睑是灰度图，分辨率应与屏幕分辨率相同（屏幕大小是脚本所固定的）。，一般眼睑图像有两套，一套是对称的，一套不是。

眼睛图像必须按以下顺序指定：巩膜、虹膜、对称上眼睑、对称下眼睑、非对称上眼睑、非对称下眼睑。接下来，需要指定虹膜的直径（在此示例中为 80 像素）。然后，将输出重定向到文件 “defaultEye.h”（建议为不同的眼睛配置使用不同的文件名）。

python tablegen.py defaultEye/sclera.png defaultEye/iris.png defaultEye/lid-upper-symmetrical.png defaultEye/lid-lower-symmetrical.png defaultEye/lid-upper.png defaultEye/lid-lower.png 80 > defaultEye.h