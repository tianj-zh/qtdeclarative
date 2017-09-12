/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qv4function_p.h"
#include "qv4functionobject_p.h"
#include "qv4managed_p.h"
#include "qv4string_p.h"
#include "qv4value_p.h"
#include "qv4engine_p.h"
#include "qv4lookup_p.h"
#include <private/qv4mm_p.h>
#include <private/qv4identifiertable_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

Function::Function(ExecutionEngine *engine, CompiledData::CompilationUnit *unit, const CompiledData::Function *function, Code codePtr)
        : compiledFunction(function)
        , compilationUnit(unit)
        , code(codePtr)
        , codeData(function->code())
        , hasQmlDependencies(function->hasQmlDependencies())
{
    Q_UNUSED(engine);

    internalClass = engine->internalClasses[EngineBase::Class_CallContext];

    // first locals
    const quint32_le *localsIndices = compiledFunction->localsTable();
    for (quint32 i = 0; i < compiledFunction->nLocals; ++i)
        internalClass = internalClass->addMember(engine->identifierTable->identifier(compilationUnit->runtimeStrings[localsIndices[i]]), Attr_NotConfigurable);

    const quint32_le *formalsIndices = compiledFunction->formalsTable();
    for (quint32 i = 0; i < compiledFunction->nFormals; ++i)
        internalClass = internalClass->addMember(engine->identifierTable->identifier(compilationUnit->runtimeStrings[formalsIndices[i]]), Attr_NotConfigurable);

    nFormals = compiledFunction->nFormals;


    canUseSimpleCall = compiledFunction->flags & CompiledData::Function::CanUseSimpleCall;
}

Function::~Function()
{
}

void Function::updateInternalClass(ExecutionEngine *engine, const QList<QByteArray> &parameters)
{
    internalClass = engine->internalClasses[EngineBase::Class_CallContext];

    // iterate backwards, so we get the right ordering for duplicate names
    Scope scope(engine);
    ScopedString arg(scope);
    for (int i = parameters.size() - 1; i >= 0; --i) {
        arg = engine->newString(QString::fromUtf8(parameters.at(i)));
        while (1) {
            InternalClass *newClass = internalClass->addMember(arg, Attr_NotConfigurable);
            if (newClass != internalClass) {
                internalClass = newClass;
                break;
            }
            // duplicate arguments, need some trick to store them
            arg = engine->memoryManager->alloc<String>(arg->d(), engine->newString(QString(0xfffe)));
        }
    }
    nFormals = parameters.size();

    const quint32_le *localsIndices = compiledFunction->localsTable();
    for (quint32 i = 0; i < compiledFunction->nLocals; ++i)
        internalClass = internalClass->addMember(engine->identifierTable->identifier(compilationUnit->runtimeStrings[localsIndices[i]]), Attr_NotConfigurable);

    canUseSimpleCall = false;
}

QT_END_NAMESPACE
