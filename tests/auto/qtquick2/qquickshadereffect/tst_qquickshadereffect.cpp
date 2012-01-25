/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>

#include <QList>
#include <QByteArray>
#include <private/qquickshadereffect_p.h>

class TestShaderEffect : public QQuickShaderEffect
{
    Q_OBJECT
    Q_PROPERTY(QVariant source READ dummyRead NOTIFY dummyChanged)
    Q_PROPERTY(QVariant _0aA9zZ READ dummyRead NOTIFY dummyChanged)
    Q_PROPERTY(QVariant x86 READ dummyRead NOTIFY dummyChanged)
    Q_PROPERTY(QVariant X READ dummyRead NOTIFY dummyChanged)

public:
    QVariant dummyRead() const { return QVariant(); }
    bool isConnected(const char *signal) const { return m_signals.contains(signal); }

protected:
    void connectNotify(const char *signal) { m_signals.append(signal); }
    void disconnectNotify(const char *signal) { m_signals.removeOne(signal); }

signals:
    void dummyChanged();

private:
    QList<QByteArray> m_signals;
};

class tst_qquickshadereffect : public QObject
{
    Q_OBJECT
public:
    tst_qquickshadereffect();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void lookThroughShaderCode_data();
    void lookThroughShaderCode();

private:
    enum PresenceFlags {
        VertexPresent = 0x01,
        TexCoordPresent = 0x02,
        MatrixPresent = 0x04,
        OpacityPresent = 0x08,
        PropertyPresent = 0x10
    };
};

tst_qquickshadereffect::tst_qquickshadereffect()
{
}

void tst_qquickshadereffect::initTestCase()
{
}

void tst_qquickshadereffect::cleanupTestCase()
{
}

void tst_qquickshadereffect::lookThroughShaderCode_data()
{
    QTest::addColumn<QByteArray>("vertexShader");
    QTest::addColumn<QByteArray>("fragmentShader");
    QTest::addColumn<int>("presenceFlags");

    QTest::newRow("default")
            << QByteArray("uniform highp mat4 qt_Matrix;                                  \n"
                          "attribute highp vec4 qt_Vertex;                                \n"
                          "attribute highp vec2 qt_MultiTexCoord0;                        \n"
                          "varying highp vec2 qt_TexCoord0;                               \n"
                          "void main() {                                                  \n"
                          "    qt_TexCoord0 = qt_MultiTexCoord0;                          \n"
                          "    gl_Position = qt_Matrix * qt_Vertex;                       \n"
                          "}")
            << QByteArray("varying highp vec2 qt_TexCoord0;                                   \n"
                          "uniform sampler2D source;                                          \n"
                          "uniform lowp float qt_Opacity;                                     \n"
                          "void main() {                                                      \n"
                          "    gl_FragColor = texture2D(source, qt_TexCoord0) * qt_Opacity;   \n"
                          "}")
            << (VertexPresent | TexCoordPresent | MatrixPresent | OpacityPresent | PropertyPresent);

    QTest::newRow("empty")
            << QByteArray(" ") // one space -- if completely empty, default will be used instead.
            << QByteArray(" ")
            << 0;


    QTest::newRow("inside line comments")
            << QByteArray("//uniform highp mat4 qt_Matrix;\n"
                          "attribute highp vec4 qt_Vertex;\n"
                          "// attribute highp vec2 qt_MultiTexCoord0;")
            << QByteArray("uniform int source; // uniform lowp float qt_Opacity;")
            << (VertexPresent | PropertyPresent);

    QTest::newRow("inside block comments")
            << QByteArray("/*uniform highp mat4 qt_Matrix;\n"
                          "*/attribute highp vec4 qt_Vertex;\n"
                          "/*/attribute highp vec2 qt_MultiTexCoord0;//**/")
            << QByteArray("/**/uniform int source; /* uniform lowp float qt_Opacity; */")
            << (VertexPresent | PropertyPresent);

    QTest::newRow("inside preprocessor directive")
            << QByteArray("#define uniform\nhighp mat4 qt_Matrix;\n"
                          "attribute highp vec4 qt_Vertex;\n"
                          "#if\\\nattribute highp vec2 qt_MultiTexCoord0;")
            << QByteArray("uniform int source;\n"
                          "    #    undef uniform lowp float qt_Opacity;")
            << (VertexPresent | PropertyPresent);


    QTest::newRow("line comments between")
            << QByteArray("uniform//foo\nhighp//bar\nmat4//baz\nqt_Matrix;\n"
                          "attribute//\nhighp//\nvec4//\nqt_Vertex;\n"
                          " //*/ uniform \n attribute //\\ \n highp //// \n vec2 //* \n qt_MultiTexCoord0;")
            << QByteArray("uniform// lowp float qt_Opacity;\nsampler2D source;")
            << (VertexPresent | TexCoordPresent | MatrixPresent | PropertyPresent);

    QTest::newRow("block comments between")
            << QByteArray("uniform/*foo*/highp/*/bar/*/mat4/**//**/qt_Matrix;\n"
                          "attribute/**/highp/**/vec4/**/qt_Vertex;\n"
                          " /* * */ attribute /*///*/ highp /****/ vec2 /**/ qt_MultiTexCoord0;")
            << QByteArray("uniform/*/ uniform//lowp/*float qt_Opacity;*/sampler2D source;")
            << (VertexPresent | TexCoordPresent | MatrixPresent | PropertyPresent);

    QTest::newRow("preprocessor directive between")
            << QByteArray("uniform\n#foo\nhighp\n#bar\nmat4\n#baz\\\nblimey\nqt_Matrix;\n"
                          "attribute\n#\nhighp\n#\nvec4\n#\nqt_Vertex;\n"
                          " #uniform \n attribute \n # foo \n highp \n #  bar \n vec2 \n#baz \n qt_MultiTexCoord0;")
            << QByteArray("uniform\n#if lowp float qt_Opacity;\nsampler2D source;")
            << (VertexPresent | TexCoordPresent | MatrixPresent | PropertyPresent);

    QTest::newRow("newline between")
            << QByteArray("uniform\nhighp\nmat4\nqt_Matrix\n;\n"
                          "attribute  \t\r\n  highp  \n  vec4  \n\n  qt_Vertex  ;\n"
                          "   \n   attribute   \n   highp   \n   vec2   \n   qt_Multi\nTexCoord0  \n  ;")
            << QByteArray("uniform\nsampler2D\nsource;"
                          "uniform lowp float qt_Opacity;")
            << (VertexPresent | MatrixPresent | OpacityPresent | PropertyPresent);


    QTest::newRow("extra characters #1")
            << QByteArray("funiform highp mat4 qt_Matrix;\n"
                          "attribute highp vec4 qt_Vertex_;\n"
                          "attribute highp vec2 qqt_MultiTexCoord0;")
            << QByteArray("uniformm int source;\n"
                          "uniform4 lowp float qt_Opacity;")
            << 0;

    QTest::newRow("extra characters #2")
            << QByteArray("attribute phighp vec4 qt_Vertex;\n"
                          "attribute highpi vec2 qt_MultiTexCoord0;"
                          "fattribute highp vec4 qt_Vertex;\n"
                          "attributed highp vec2 qt_MultiTexCoord0;")
            << QByteArray(" ")
            << 0;

    QTest::newRow("missing characters #1")
            << QByteArray("unifor highp mat4 qt_Matrix;\n"
                          "attribute highp vec4 qt_Vert;\n"
                          "attribute highp vec2 MultiTexCoord0;")
            << QByteArray("niform int source;\n"
                          "uniform qt_Opacity;")
            << 0;

    QTest::newRow("missing characters #2")
            << QByteArray("attribute high vec4 qt_Vertex;\n"
                          "attribute ighp vec2 qt_MultiTexCoord0;"
                          "tribute highp vec4 qt_Vertex;\n"
                          "attrib highp vec2 qt_MultiTexCoord0;")
            << QByteArray(" ")
            << 0;

    QTest::newRow("precision")
            << QByteArray("uniform mat4 qt_Matrix;\n"
                          "attribute kindofhighp vec4 qt_Vertex;\n"
                          "attribute highp qt_MultiTexCoord0;\n")
            << QByteArray("uniform lowp float qt_Opacity;\n"
                          "uniform mediump float source;\n")
            << (MatrixPresent | OpacityPresent | PropertyPresent);


    QTest::newRow("property name #1")
            << QByteArray("uniform highp vec3 _0aA9zZ;")
            << QByteArray(" ")
            << int(PropertyPresent);

    QTest::newRow("property name #2")
            << QByteArray("uniform mediump vec2 x86;")
            << QByteArray(" ")
            << int(PropertyPresent);

    QTest::newRow("property name #3")
            << QByteArray("uniform lowp float X;")
            << QByteArray(" ")
            << int(PropertyPresent);
}

void tst_qquickshadereffect::lookThroughShaderCode()
{
    QFETCH(QByteArray, vertexShader);
    QFETCH(QByteArray, fragmentShader);
    QFETCH(int, presenceFlags);

    TestShaderEffect item;
    QVERIFY(!item.isConnected(SIGNAL(dummyChanged()))); // Nothing connected yet.

    QString expected;
    if ((presenceFlags & VertexPresent) == 0)
        expected += "Warning: Missing reference to \'qt_Vertex\'.\n";
    if ((presenceFlags & TexCoordPresent) == 0)
        expected += "Warning: Missing reference to \'qt_MultiTexCoord0\'.\n";
    if ((presenceFlags & MatrixPresent) == 0)
        expected += "Warning: Missing reference to \'qt_Matrix\'.\n";
    if ((presenceFlags & OpacityPresent) == 0)
        expected += "Warning: Missing reference to \'qt_Opacity\'.\n";

    item.setVertexShader(vertexShader);
    item.setFragmentShader(fragmentShader);
    item.ensureCompleted();
    QCOMPARE(item.parseLog(), expected);

    // If the uniform was successfully parsed, the notify signal has been connected to an update slot.
    QCOMPARE(item.isConnected(SIGNAL(dummyChanged())), (presenceFlags & PropertyPresent) != 0);
}

QTEST_MAIN(tst_qquickshadereffect)

#include "tst_qquickshadereffect.moc"
