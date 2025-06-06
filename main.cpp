#include <QApplication>
#include <QSurfaceFormat> // ���ͷ�ļ�
#include "src/include/mainwindow.h"

int main(int argc, char *argv[]) {
    // ǿ��ʹ�� ANGLE (OpenGL ES on DirectX) ������ OpenGL
    // QCoreApplication::setAttribute(Qt::AA_UseOpenGLES); // ����
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL); // ��������OpenGL

    QSurfaceFormat fmt;
    // ����OpenGL 3.2���������ļ�����߰汾���������
    fmt.setVersion(3, 2);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    // fmt.setRenderableType(QSurfaceFormat::OpenGL); // ��ȷָ��OpenGL
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);

    MainWindow window;
    window.setWindowTitle("ϵͳ���ܼ�ع���"); // �޸�Ϊ��ȷ������
    window.resize(1500, 900);
    window.show();

    return app.exec();
}
