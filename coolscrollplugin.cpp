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

#include "coolscrollplugin.h"
#include "coolscrollconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <texteditor/texteditor.h>

#include <QMainWindow>
#include <QScrollBar>
#include <QPushButton>

#include <QtCore/QtPlugin>
#include <iostream>

#include "coolscrollbarsettings.h"
#include "coolscrollbar.h"
#include "settingspage.h"

namespace
{
    const QString l_nSettingsGroup(QStringLiteral("CoolScroll"));
}

using namespace CoolScroll::Internal;

CoolScrollPlugin::CoolScrollPlugin() :
    ExtensionSystem::IPlugin(),
    m_settings(new CoolScrollbarSettings)
{
    readSettings();
}

CoolScrollPlugin::~CoolScrollPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool CoolScrollPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    SettingsPage* settingsPage = new SettingsPage(m_settings);
    connect(settingsPage, SIGNAL(settingsChanged()), SLOT(settingChanged()));
    addAutoReleasedObject(settingsPage);

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
                                               SLOT(currentEditorChanged(Core::IEditor*)));
    connect(Core::EditorManager::instance(), SIGNAL(editorAboutToClose(Core::IEditor*)),
                                               SLOT(editorAboutToClose(Core::IEditor*)));
    connect(Core::EditorManager::instance(), SIGNAL(editorCreated(Core::IEditor*, QString)),
                                               SLOT(editorCreated(Core::IEditor*, QString)));
    connect(Core::EditorManager::instance(), SIGNAL(currentEditorAboutToChange(Core::IEditor*)),
                                               SLOT(currentEditorAboutToChange(Core::IEditor*)));

    return true;
}

void CoolScrollPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // "In the extensionsInitialized method, a plugin can be sure that all
    //  plugins that depend on it are completely initialized."
}

ExtensionSystem::IPlugin::ShutdownFlag CoolScrollPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    disconnect(Core::EditorManager::instance(), 0, this, 0);
    m_openedEditorsScrollbarsMap.clear();
    saveSettings();
    return SynchronousShutdown;
}


void CoolScrollPlugin::editorCreated(Core::IEditor *editor, const QString &fileName)
{
    Q_UNUSED(fileName);

    TextEditor::TextEditorWidget* newEditor = qobject_cast<TextEditor::TextEditorWidget*>(editor->widget());
    if (newEditor)
    {
        CoolScrollBar* newScrollBar = new CoolScrollBar(newEditor, m_settings);
        m_openedEditorsScrollbarsMap.insert( { editor , newScrollBar } );
        newEditor->setVerticalScrollBar(newScrollBar);
    }
}

void CoolScrollPlugin::currentEditorAboutToChange(Core::IEditor *editor)
{
    if (m_openedEditorsScrollbarsMap.find(editor) != m_openedEditorsScrollbarsMap.end())
    {
        qDebug() << "deactivate...";
        m_openedEditorsScrollbarsMap[editor]->deactivate();
    }
}

void CoolScrollPlugin::currentEditorChanged(Core::IEditor *editor)
{
    if (editor != nullptr)
    {
        auto lookupIter = m_openedEditorsScrollbarsMap.find(editor);
        if (lookupIter != m_openedEditorsScrollbarsMap.end())
        {
            lookupIter->second->activate();
        }
    }
}

void CoolScrollPlugin::editorAboutToClose(Core::IEditor *editor)
{
    auto lookupIter = m_openedEditorsScrollbarsMap.find(editor);
    if (lookupIter != m_openedEditorsScrollbarsMap.end())
    {
        m_openedEditorsScrollbarsMap.erase(lookupIter);
    }
}

void CoolScroll::Internal::CoolScrollPlugin::readSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(l_nSettingsGroup);
    m_settings->read(settings);
    settings->endGroup();
}

void CoolScroll::Internal::CoolScrollPlugin::saveSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(l_nSettingsGroup);
    m_settings->save(settings);
    settings->endGroup();
    settings->sync();
}

CoolScrollBar *CoolScrollPlugin::scrollBarForEditor(Core::IEditor *editor)
{
    if (m_openedEditorsScrollbarsMap.find(editor) != m_openedEditorsScrollbarsMap.end())
    {
        return m_openedEditorsScrollbarsMap[editor];
    }
    else
    {
        return nullptr;
    }
}


void CoolScroll::Internal::CoolScrollPlugin::settingChanged()
{
    saveSettings();
    Core::EditorManager* em = Core::EditorManager::instance();
    auto lookupCurrent = m_openedEditorsScrollbarsMap.find(em->currentEditor());
    if (lookupCurrent != m_openedEditorsScrollbarsMap.end())
    {
        lookupCurrent->second->applySettings();
    }

//    QList<Core::IEditor*> editors = em->documentModel()->oneEditorForEachOpenedDocument();
//    QList<Core::IEditor*>::iterator it = editors.begin();
//    // editors will update settings after next opening
//    for( ; it != editors.end(); ++it)
//    {
//        CoolScrollBar* bar = getEditorScrollBar(*it);
//        Q_ASSERT(bar);
//        bar->markStateDirty();
//    }
//    // update current editor right now
//    CoolScrollBar* bar = getEditorScrollBar(em->currentEditor());
//    Q_ASSERT(bar);
//    bar->fullUpdateSettings();
//    mCoolScrollBar->fullUpdateSettings();
}

//Q_EXPORT_PLUGIN2(CoolScroll, CoolScrollPlugin)
