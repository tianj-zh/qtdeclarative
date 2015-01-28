/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QString>
#include <QtTest/QtTest>

#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>

#include <QtQuick/qsgnode.h>
#include <QtQuick/private/qsgbatchrenderer_p.h>
#include <QtQuick/private/qsgnodeupdater_p.h>
#include <QtQuick/private/qsgrenderloop_p.h>
#include <QtQuick/private/qsgcontext_p.h>

#include <QtQuick/qsgsimplerectnode.h>
#include <QtQuick/qsgsimpletexturenode.h>
#include <QtQuick/private/qsgtexture_p.h>

QT_BEGIN_NAMESPACE
inline bool operator==(const QSGGeometry::TexturedPoint2D& l, const QSGGeometry::TexturedPoint2D& r)
{
    return l.x == r.x && l.y == r.y && l.tx == r.tx && l.ty == r.ty;
}
QT_END_NAMESPACE

class NodesTest : public QObject
{
    Q_OBJECT

public:
    NodesTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    // Root nodes
    void propegate();
    void propegateWithMultipleRoots();

    // Opacity nodes
    void basicOpacityNode();
    void opacityPropegation();

    void isBlockedCheck();

    void textureNodeTextureOwnership();
    void textureNodeRect();

private:
    QOffscreenSurface *surface;
    QOpenGLContext *context;
    QSGRenderContext *renderContext;
};

void NodesTest::initTestCase()
{
    QSGRenderLoop *renderLoop = QSGRenderLoop::instance();

    surface = new QOffscreenSurface;
    surface->create();

    context = new QOpenGLContext();
    context->create();
    context->makeCurrent(surface);

    renderContext = renderLoop->createRenderContext(renderLoop->sceneGraphContext());
    renderContext->initialize(context);
}

void NodesTest::cleanupTestCase()
{
    renderContext->invalidate();
    context->doneCurrent();
    delete context;
    delete surface;
}

class DummyRenderer : public QSGBatchRenderer::Renderer
{
public:
    DummyRenderer(QSGRootNode *root, QSGRenderContext *renderContext)
        : QSGBatchRenderer::Renderer(renderContext)
        , changedNode(0)
        , changedState(0)
        , renderCount(0)
    {
        setRootNode(root);
    }

    void render() {
        ++renderCount;
        renderingOrder = ++globalRendereringOrder;
    }

    void nodeChanged(QSGNode *node, QSGNode::DirtyState state) {
        changedNode = node;
        changedState = state;
        QSGBatchRenderer::Renderer::nodeChanged(node, state);
    }

    QSGNode *changedNode;
    QSGNode::DirtyState changedState;

    int renderCount;
    int renderingOrder;
    static int globalRendereringOrder;
};

int DummyRenderer::globalRendereringOrder;

NodesTest::NodesTest()
{
}


void NodesTest::propegate()
{
    QSGRootNode root;
    QSGNode child; child.setFlag(QSGNode::OwnedByParent, false);
    root.appendChildNode(&child);

    DummyRenderer renderer(&root, renderContext);

    child.markDirty(QSGNode::DirtyGeometry);

    QCOMPARE(&child, renderer.changedNode);
    QCOMPARE((int) renderer.changedState, (int) QSGNode::DirtyGeometry);
}


void NodesTest::propegateWithMultipleRoots()
{
    QSGRootNode root1;
    QSGNode child2; child2.setFlag(QSGNode::OwnedByParent, false);
    QSGRootNode root3; root3.setFlag(QSGNode::OwnedByParent, false);
    QSGNode child4; child4.setFlag(QSGNode::OwnedByParent, false);

    root1.appendChildNode(&child2);
    child2.appendChildNode(&root3);
    root3.appendChildNode(&child4);

    DummyRenderer ren1(&root1, renderContext);
    DummyRenderer ren2(&root3, renderContext);

    child4.markDirty(QSGNode::DirtyGeometry);

    QCOMPARE(ren1.changedNode, &child4);
    QCOMPARE(ren2.changedNode, &child4);

    QCOMPARE((int) ren1.changedState, (int) QSGNode::DirtyGeometry);
    QCOMPARE((int) ren2.changedState, (int) QSGNode::DirtyGeometry);
}

void NodesTest::basicOpacityNode()
{
    QSGOpacityNode n;
    QCOMPARE(n.opacity(), 1.);

    n.setOpacity(0.5);
    QCOMPARE(n.opacity(), 0.5);

    n.setOpacity(-1);
    QCOMPARE(n.opacity(), 0.);

    n.setOpacity(2);
    QCOMPARE(n.opacity(), 1.);
}

void NodesTest::opacityPropegation()
{
    QSGRootNode root;
    QSGOpacityNode *a = new QSGOpacityNode;
    QSGOpacityNode *b = new QSGOpacityNode;
    QSGOpacityNode *c = new QSGOpacityNode;

    QSGSimpleRectNode *geometry = new QSGSimpleRectNode;
    geometry->setRect(0, 0, 100, 100);

    DummyRenderer renderer(&root, renderContext);

    root.appendChildNode(a);
    a->appendChildNode(b);
    b->appendChildNode(c);
    c->appendChildNode(geometry);

    a->setOpacity(0.9);
    b->setOpacity(0.8);
    c->setOpacity(0.7);

    renderer.renderScene();

    QCOMPARE(a->combinedOpacity(), 0.9);
    QCOMPARE(b->combinedOpacity(), 0.9 * 0.8);
    QCOMPARE(c->combinedOpacity(), 0.9 * 0.8 * 0.7);
    QCOMPARE(geometry->inheritedOpacity(), 0.9 * 0.8 * 0.7);

    b->setOpacity(0.1);
    renderer.renderScene();

    QCOMPARE(a->combinedOpacity(), 0.9);
    QCOMPARE(b->combinedOpacity(), 0.9 * 0.1);
    QCOMPARE(c->combinedOpacity(), 0.9 * 0.1 * 0.7);
    QCOMPARE(geometry->inheritedOpacity(), 0.9 * 0.1 * 0.7);

    b->setOpacity(0);
    renderer.renderScene();

    QVERIFY(b->isSubtreeBlocked());

    // Verify that geometry did not get updated as it is in a blocked
    // subtree
    QCOMPARE(c->combinedOpacity(), 0.9 * 0.1 * 0.7);
    QCOMPARE(geometry->inheritedOpacity(), 0.9 * 0.1 * 0.7);
}

void NodesTest::isBlockedCheck()
{
    QSGRootNode root;
    QSGOpacityNode *opacity = new QSGOpacityNode();
    QSGNode *node = new QSGNode();

    root.appendChildNode(opacity);
    opacity->appendChildNode(node);

    QSGNodeUpdater updater;

    opacity->setOpacity(0);
    QVERIFY(updater.isNodeBlocked(node, &root));

    opacity->setOpacity(1);
    QVERIFY(!updater.isNodeBlocked(node, &root));
}

void NodesTest::textureNodeTextureOwnership()
{
    { // Check that it is not deleted by default
        QPointer<QSGTexture> texture(new QSGPlainTexture());

        QSGSimpleTextureNode *tn = new QSGSimpleTextureNode();
        QVERIFY(!tn->ownsTexture());

        tn->setTexture(texture);
        delete tn;
        QVERIFY(!texture.isNull());
    }

    { // Check that it is deleted when we so desire
        QPointer<QSGTexture> texture(new QSGPlainTexture());

        QSGSimpleTextureNode *tn = new QSGSimpleTextureNode();
        tn->setOwnsTexture(true);
        QVERIFY(tn->ownsTexture());

        tn->setTexture(texture);
        delete tn;
        QVERIFY(texture.isNull());
    }
}

void NodesTest::textureNodeRect()
{
    QSGPlainTexture texture;
    texture.setTextureSize(QSize(400, 400));
    QSGSimpleTextureNode tn;
    tn.setTexture(&texture);
    QSGGeometry::TexturedPoint2D *vertices = tn.geometry()->vertexDataAsTexturedPoint2D();

    QSGGeometry::TexturedPoint2D topLeft, bottomLeft, topRight, bottomRight;
    topLeft.set(0, 0, 0, 0);
    bottomLeft.set(0, 0, 0, 1);
    topRight.set(0, 0, 1, 0);
    bottomRight.set(0, 0, 1, 1);
    QCOMPARE(vertices[0], topLeft);
    QCOMPARE(vertices[1], bottomLeft);
    QCOMPARE(vertices[2], topRight);
    QCOMPARE(vertices[3], bottomRight);

    tn.setRect(1, 2, 100, 100);
    topLeft.set(1, 2, 0, 0);
    bottomLeft.set(1, 102, 0, 1);
    topRight.set(101, 2, 1, 0);
    bottomRight.set(101, 102, 1, 1);
    QCOMPARE(vertices[0], topLeft);
    QCOMPARE(vertices[1], bottomLeft);
    QCOMPARE(vertices[2], topRight);
    QCOMPARE(vertices[3], bottomRight);

    tn.setRect(0, 0, 100, 100);
    tn.setSourceRect(100, 100, 200, 200);
    topLeft.set(0, 0, 0.25, 0.25);
    bottomLeft.set(0, 100, 0.25, 0.75);
    topRight.set(100, 0, 0.75, 0.25);
    bottomRight.set(100, 100, 0.75, 0.75);
    QCOMPARE(vertices[0], topLeft);
    QCOMPARE(vertices[1], bottomLeft);
    QCOMPARE(vertices[2], topRight);
    QCOMPARE(vertices[3], bottomRight);

    tn.setSourceRect(300, 300, -200, -200);
    topLeft.set(0, 0, 0.75, 0.75);
    bottomLeft.set(0, 100, 0.75, 0.25);
    topRight.set(100, 0, 0.25, 0.75);
    bottomRight.set(100, 100, 0.25, 0.25);
    QCOMPARE(vertices[0], topLeft);
    QCOMPARE(vertices[1], bottomLeft);
    QCOMPARE(vertices[2], topRight);
    QCOMPARE(vertices[3], bottomRight);
}

QTEST_MAIN(NodesTest);

#include "tst_nodestest.moc"
