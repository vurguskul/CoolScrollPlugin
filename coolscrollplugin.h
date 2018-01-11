/*
*
* Copyright (C) 2011 EgorZhuk
*
* Authors: Egor Zhuk <egor.zhuk@gmail.com>
*
* This file is part of CoolScroll plugin for QtCreator.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*/

#ifndef COOLSCROLL_H
#define COOLSCROLL_H

#include "coolscroll_global.h"

#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QSharedPointer>
#include <unordered_map>

class CoolScrollbarSettings;
class CoolScrollBar;
class QScrollBar;

namespace CoolScroll {
namespace Internal {

class CoolScrollPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CoolScrollPlugin.json")
public:
    CoolScrollPlugin();
    ~CoolScrollPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

private:
    void readSettings();
    void saveSettings();

    QSharedPointer<CoolScrollbarSettings> m_settings;

    CoolScrollBar* scrollBarForEditor(Core::IEditor* editor);

    std::unordered_map<Core::IEditor*, CoolScrollBar*> m_openedEditorsScrollbarsMap;

private slots:

    void editorCreated(Core::IEditor* editor, const QString& fileName);
    void currentEditorAboutToChange(Core::IEditor *editor);
    void currentEditorChanged(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);
    void settingChanged();
};

} // namespace Internal
} // namespace CoolScroll

#endif // COOLSCROLL_H

