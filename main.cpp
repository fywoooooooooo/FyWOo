#include <QApplication>
#include <QSurfaceFormat> // 添加头文件
#include "src/include/mainwindow.h"

int main(int argc, char *argv[]) {
    // 强制使用 ANGLE (OpenGL ES on DirectX) 或桌面 OpenGL
    // QCoreApplication::setAttribute(Qt::AA_UseOpenGLES); // 或者
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL); // 尝试桌面OpenGL

    QSurfaceFormat fmt;
    // 请求OpenGL 3.2核心配置文件或更高版本，如果可用
    fmt.setVersion(3, 2);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    // fmt.setRenderableType(QSurfaceFormat::OpenGL); // 明确指定OpenGL
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);

    MainWindow window;
    window.setWindowTitle("系统性能监控工具"); // 修改为正确的中文
    window.resize(1500, 900);
    window.show();

    return app.exec();
}
