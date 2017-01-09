/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKPATHITEM_P_P_H
#define QQUICKPATHITEM_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickpathitem_p.h"
#include "qquickitem_p.h"
#include <QPainterPath>
#include <QColor>
#include <QBrush>
#include <private/qopenglcontext_p.h>

QT_BEGIN_NAMESPACE

class QSGPlainTexture;

class QQuickAbstractPathRenderer
{
public:
    virtual ~QQuickAbstractPathRenderer() { }

    // Gui thread
    virtual void beginSync(int totalCount) = 0;
    virtual void setPath(int index, const QQuickPath *path) = 0;
    virtual void setStrokeColor(int index, const QColor &color) = 0;
    virtual void setStrokeWidth(int index, qreal w) = 0;
    virtual void setFillColor(int index, const QColor &color) = 0;
    virtual void setFillRule(int index, QQuickVisualPath::FillRule fillRule) = 0;
    virtual void setJoinStyle(int index, QQuickVisualPath::JoinStyle joinStyle, int miterLimit) = 0;
    virtual void setCapStyle(int index, QQuickVisualPath::CapStyle capStyle) = 0;
    virtual void setStrokeStyle(int index, QQuickVisualPath::StrokeStyle strokeStyle,
                                qreal dashOffset, const QVector<qreal> &dashPattern) = 0;
    virtual void setFillGradient(int index, QQuickPathGradient *gradient) = 0;
    virtual void endSync(bool async) = 0;

    // Render thread, with gui blocked
    virtual void updateNode() = 0;
};

class QQuickVisualPathPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickVisualPath)

public:
    enum Dirty {
        DirtyPath = 0x01,
        DirtyStrokeColor = 0x02,
        DirtyStrokeWidth = 0x04,
        DirtyFillColor = 0x08,
        DirtyFillRule = 0x10,
        DirtyStyle = 0x20,
        DirtyDash = 0x40,
        DirtyFillGradient = 0x80,

        DirtyAll = 0xFF
    };

    QQuickVisualPathPrivate();

    void _q_pathChanged();
    void _q_fillGradientChanged();

    static QQuickVisualPathPrivate *get(QQuickVisualPath *p) { return p->d_func(); }

    QQuickPath *path;
    int dirty;
    QColor strokeColor;
    qreal strokeWidth;
    QColor fillColor;
    QQuickVisualPath::FillRule fillRule;
    QQuickVisualPath::JoinStyle joinStyle;
    int miterLimit;
    QQuickVisualPath::CapStyle capStyle;
    QQuickVisualPath::StrokeStyle strokeStyle;
    qreal dashOffset;
    QVector<qreal> dashPattern;
    QQuickPathGradient *fillGradient;
};

class QQuickPathItemPrivate : public QQuickItemPrivate
{
    Q_DECLARE_PUBLIC(QQuickPathItem)

public:
    QQuickPathItemPrivate();
    ~QQuickPathItemPrivate();

    void createRenderer();
    QSGNode *createNode();
    void sync();

    void _q_visualPathChanged();

    static QQuickPathItemPrivate *get(QQuickPathItem *item) { return item->d_func(); }

    bool componentComplete;
    bool vpChanged;
    QQuickPathItem::RendererType rendererType;
    bool async;
    QQuickAbstractPathRenderer *renderer;
    QVector<QQuickVisualPath *> vp;
};

#ifndef QT_NO_OPENGL

class QQuickPathItemGradientCache : public QOpenGLSharedResource
{
public:
    struct GradientDesc {
        QGradientStops stops;
        QPointF start;
        QPointF end;
        QQuickPathGradient::SpreadMode spread;
        bool operator==(const GradientDesc &other) const
        {
            return start == other.start && end == other.end && spread == other.spread
                   && stops == other.stops;
        }
    };

    QQuickPathItemGradientCache(QOpenGLContext *context) : QOpenGLSharedResource(context->shareGroup()) { }
    ~QQuickPathItemGradientCache();

    void invalidateResource() override;
    void freeResource(QOpenGLContext *) override;

    QSGTexture *get(const GradientDesc &grad);

    static QQuickPathItemGradientCache *currentCache();

private:
    QHash<GradientDesc, QSGPlainTexture *> m_cache;
};

inline uint qHash(const QQuickPathItemGradientCache::GradientDesc &v, uint seed = 0)
{
    uint h = seed;
    h += v.start.x() + v.end.y() + v.spread;
    for (int i = 0; i < 3 && i < v.stops.count(); ++i)
        h += v.stops[i].second.rgba();
    return h;
}

#endif // QT_NO_OPENGL

QT_END_NAMESPACE

#endif