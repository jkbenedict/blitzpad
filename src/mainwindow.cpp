#include "mainwindow.h"
#include "editor.h"
#include "languages.h"
#include "finddialog.h"
#include "search.h"
#include "compress.h"
#include "prefs.h"
#include "theme.h"
#include "optionsdialogs.h"
#include "fileformatdialog.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFile>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPointer>
#include <QCheckBox>
#include <QRegularExpression>
#include <QHeaderView>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QInputDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTreeWidget>

#ifdef __GLIBC__
#include <malloc.h>
#endif
#include <unistd.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // First run (or first run of a new version): write every setting into
    // ~/.blitzpad/preferences.ini so the file lists everything you can change.
    if (Prefs::settings().value("meta/defaults-written-for").toString() != QStringLiteral(BLITZPAD_VERSION)
        || !Prefs::settings().contains("colours/text-fg")) {
        Theme::writeAllDefaults();
        Prefs::settings().setValue("meta/defaults-written-for", QStringLiteral(BLITZPAD_VERSION));
    }
    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->tabBar()->setExpanding(false);
    m_tabs->tabBar()->setElideMode(Qt::ElideMiddle);
    setCentralWidget(m_tabs);

    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);
    // Double-click on the empty tab bar area opens a new tab, like Notepad++.
    connect(m_tabs->tabBar(), &QTabBar::tabBarDoubleClicked, this, [this](int i) {
        if (i < 0) newTab();
    });

    m_posLabel = new QLabel(this);
    m_langLabel = new QLabel(this);
    m_encLabel = new QLabel(this);
    statusBar()->addWidget(m_langLabel, 1);
    statusBar()->addPermanentWidget(m_posLabel);
    statusBar()->addPermanentWidget(m_encLabel);

    buildMenus();
    buildSearch();
    buildAbout();
    buildOptions();
    applyShortcuts();

    // "X Tabs | Y MEM" on the far right of the menu bar. Refreshed when a tab
    // opens or closes, and every 60 seconds.
    m_statsLabel = new QLabel(this);
    m_statsLabel->setContentsMargins(8, 0, 10, 0);
    m_statsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    // The menu bar sizes its corner widget once, so reserve room for big numbers.
    m_statsLabel->setMinimumWidth(m_statsLabel->fontMetrics().horizontalAdvance("9999 Tabs | 99999 MEM") + 18);
    menuBar()->setCornerWidget(m_statsLabel, Qt::TopRightCorner);
    m_statsTimer = new QTimer(this);
    m_statsTimer->setInterval(60 * 1000);
    connect(m_statsTimer, &QTimer::timeout, this, &MainWindow::updateTabMemStats);
    m_statsTimer->start();

    resize(1000, 700);
    restoreGeometry(Prefs::settings().value("window/geometry").toByteArray());
}

void MainWindow::buildMenus()
{
    QMenu *file = menuBar()->addMenu(tr("&File"));
    m_fileMenu = file;
    file->addAction(tr("&New"), QKeySequence::New, this, [this] { newTab(); });
    file->addAction(tr("&Open..."), QKeySequence::Open, this, &MainWindow::open);
    file->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::save);
    // Explicit shortcuts: QKeySequence::SaveAs is empty on some desktops.
    // Ctrl+Alt+S matches Notepad++, Ctrl+Shift+S matches most Linux apps.
    QAction *saveAsAct = file->addAction(tr("Save &As..."), this, &MainWindow::saveAs);
    saveAsAct->setShortcuts({QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S)});
    file->addAction(tr("Save &Compressed..."), this, &MainWindow::saveCompressed);
    file->addSeparator();
    file->addAction(tr("&Close Tab"), QKeySequence(Qt::CTRL | Qt::Key_W), this,
                    [this] { closeTab(m_tabs->currentIndex()); });
    // Alt+F4 like Notepad++ (Ctrl+Q is Toggle Comment there and here).
    file->addAction(tr("E&xit"), QKeySequence(Qt::ALT | Qt::Key_F4), this, &QWidget::close);

    QMenu *edit = menuBar()->addMenu(tr("&Edit"));
    edit->addAction(tr("&Undo"), QKeySequence::Undo, this, [this] { if (auto e = current()) e->undo(); });
    QAction *redo = edit->addAction(tr("&Redo"), this, [this] { if (auto e = current()) e->redo(); });
    redo->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Y), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)});
    edit->addSeparator();
    edit->addAction(tr("Cu&t"), QKeySequence::Cut, this, [this] { if (auto e = current()) e->cut(); });
    edit->addAction(tr("&Copy"), QKeySequence::Copy, this, [this] { if (auto e = current()) e->copy(); });
    edit->addAction(tr("&Paste"), QKeySequence::Paste, this, [this] { if (auto e = current()) e->paste(); });
    edit->addAction(tr("Select &All"), QKeySequence::SelectAll, this, [this] { if (auto e = current()) e->selectAll(); });
    edit->addSeparator();
    edit->addAction(tr("Duplicate Line"), QKeySequence(Qt::CTRL | Qt::Key_D), this,
                    [this] { if (auto e = current()) e->SendScintilla(QsciScintilla::SCI_LINEDUPLICATE); });
    edit->addAction(tr("Toggle Comment"), QKeySequence(Qt::CTRL | Qt::Key_Q), this, [this] {
        // Uses the language's line-comment marker (your own formats included).
        Editor *e = current();
        if (!e) return;
        const Language *L = Languages::find(e->language());
        if (!L || L->lineComment.isEmpty()) {
            statusBar()->showMessage(tr("%1 has no line comments.").arg(e->language()), 3000);
            return;
        }
        const QString tok = L->lineComment + " ";
        int l1, c1, l2, c2;
        e->getSelection(&l1, &c1, &l2, &c2);
        if (l1 < 0) { e->getCursorPosition(&l1, &c1); l2 = l1; c2 = c1; }
        if (l2 > l1 && c2 == 0) --l2;
        e->beginUndoAction();
        for (int l = l1; l <= l2; ++l) {
            const QString text = e->text(l);
            int indent = 0;
            while (indent < text.size() && (text.at(indent) == ' ' || text.at(indent) == '\t')) ++indent;
            const QString trimmed = text.trimmed();
            if (trimmed.startsWith(tok.trimmed())) {
                int col = text.indexOf(tok.trimmed());
                int n = text.mid(col).startsWith(tok) ? tok.size() : tok.trimmed().size();
                e->setSelection(l, col, l, col + n);
                e->removeSelectedText();
            } else if (!trimmed.isEmpty()) {
                e->insertAt(tok, l, qMax(0, indent));
            }
        }
        e->endUndoAction();
    });

    QMenu *view = menuBar()->addMenu(tr("&View"));
    m_wrapAction = view->addAction(tr("&Word Wrap"));
    m_wrapAction->setCheckable(true);
    connect(m_wrapAction, &QAction::toggled, this, [this](bool on) {
        if (auto e = current()) e->setWrapMode(on ? QsciScintilla::WrapWord : QsciScintilla::WrapNone);
        Prefs::settings().setValue("view/wordWrap", on); // default for new tabs
    });
    view->addAction(tr("Zoom &In"), QKeySequence::ZoomIn, this, [this] { if (auto e = current()) e->zoomIn(); });
    view->addAction(tr("Zoom &Out"), QKeySequence::ZoomOut, this, [this] { if (auto e = current()) e->zoomOut(); });
    view->addAction(tr("Reset Zoom"), QKeySequence(Qt::CTRL | Qt::Key_0), this, [this] { if (auto e = current()) e->zoomTo(0); });
    view->addSeparator();
    auto step = [this](int d) {
        const int n = m_tabs->count();
        if (n > 1) m_tabs->setCurrentIndex((m_tabs->currentIndex() + d + n) % n);
    };
    QAction *next = view->addAction(tr("Next Tab"), this, [step] { step(1); });
    next->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Tab), QKeySequence(Qt::CTRL | Qt::Key_PageDown)});
    QAction *prev = view->addAction(tr("Previous Tab"), this, [step] { step(-1); });
    prev->setShortcuts({QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), QKeySequence(Qt::CTRL | Qt::Key_PageUp)});
    view->addSeparator();
    // Qt's own file dialog is the default. The desktop's native dialog (GTK or
    // xdg-desktop-portal) can fail to appear for Qt apps on some GNOME/Zorin
    // setups, especially under Wayland, so it is opt-in.
    QAction *native = view->addAction(tr("Use System File &Dialogs"));
    native->setCheckable(true);
    native->setChecked(Prefs::get("view/useNativeFileDialogs", false).toBool());
    connect(native, &QAction::toggled, this, [](bool on) { Prefs::settings().setValue("view/useNativeFileDialogs", on); Prefs::settings().sync(); });
    view->addSeparator();
    QAction *ws = view->addAction(tr("Show &Whitespace and Line Endings"));
    ws->setCheckable(true);
    m_showWhitespace = Prefs::get("view/showWhitespace", false).toBool();
    ws->setChecked(m_showWhitespace);
    connect(ws, &QAction::toggled, this, [this](bool on) {
        m_showWhitespace = on;
        Prefs::settings().setValue("view/showWhitespace", on);
        for (int i = 0; i < m_tabs->count(); ++i) {
            editorAt(i)->setWhitespaceVisibility(on ? QsciScintilla::WsVisible : QsciScintilla::WsInvisible);
            editorAt(i)->setEolVisibility(on);
        }
    });

    m_langMenu = menuBar()->addMenu(tr("&Language"));
    m_langGroup = new QActionGroup(this);
    rebuildLanguageMenu();
    connect(m_langGroup, &QActionGroup::triggered, this, [this](QAction *a) {
        if (auto e = current()) { e->setLanguage(a->data().toString()); updateStatus(); }
    });
}

Editor *MainWindow::editorAt(int index) const
{
    return qobject_cast<Editor *>(m_tabs->widget(index));
}

Editor *MainWindow::current() const
{
    return qobject_cast<Editor *>(m_tabs->currentWidget());
}

Editor *MainWindow::newTab()
{
    auto *ed = new Editor(m_tabs);
    connect(ed, &QsciScintilla::cursorPositionChanged, this, &MainWindow::updateStatus);
    connect(ed, &QsciScintilla::modificationChanged, this, [this, ed] { updateTabTitle(ed); });
    if (Prefs::get("view/wordWrap", false).toBool())
        ed->setWrapMode(QsciScintilla::WrapWord);
    if (m_showWhitespace) {
        ed->setWhitespaceVisibility(QsciScintilla::WsVisible);
        ed->setEolVisibility(true);
    }
    const int i = m_tabs->addTab(ed, ed->displayName());
    m_tabs->setCurrentIndex(i);
    ed->setFocus();
    // Refresh once the event loop has run, so the new tab is counted fully.
    QTimer::singleShot(0, this, &MainWindow::updateTabMemStats);
    return ed;
}

bool MainWindow::openFile(const QString &path)
{
    const QString abs = QFileInfo(path).absoluteFilePath();
    // Already open? Just switch to it.
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (editorAt(i)->filePath() == abs) {
            m_tabs->setCurrentIndex(i);
            return true;
        }
    }

    // Reuse a single untouched "new" tab instead of leaving it behind.
    Editor *ed = current();
    if (!(ed && ed->filePath().isEmpty() && !ed->isModified() && ed->length() == 0 && m_tabs->count() == 1))
        ed = newTab();

    QString err;
    if (!QFileInfo::exists(abs)) {
        // Like most editors: a missing path opens an empty tab that saves there.
        if (!ed->saveFile(abs, &err)) {
            QMessageBox::warning(this, tr("Open"), tr("Cannot create %1:\n%2").arg(abs, err));
            return false;
        }
    } else if (!ed->loadFile(abs, &err)) {
        QMessageBox::warning(this, tr("Open"), tr("Cannot open %1:\n%2").arg(abs, err));
        if (m_tabs->count() > 1) closeTab(m_tabs->indexOf(ed));
        return false;
    }
    updateTabTitle(ed);
    m_tabs->setTabToolTip(m_tabs->indexOf(ed), abs);
    updateStatus();
    QTimer::singleShot(0, this, &MainWindow::updateTabMemStats);
    if (!ed->unknownExtension().isEmpty()) {
        // Ask once the window is up (files given on the command line too).
        QPointer<Editor> p(ed);
        QTimer::singleShot(0, this, [this, p] { if (p) askAboutUnknownFormat(p); });
    }
    return true;
}

void MainWindow::open()
{
    QString dir;
    if (Editor *e = current(); e && !e->filePath().isEmpty())
        dir = QFileInfo(e->filePath()).absolutePath();
    const QStringList files = QFileDialog::getOpenFileNames(this, tr("Open"), dir, QString(), nullptr, fileDialogOptions());
    for (const QString &f : files)
        openFile(f);
}

bool MainWindow::saveEditor(Editor *ed, bool forceAsk)
{
    QString path = ed->filePath();
    if (path.isEmpty() || forceAsk) {
        path = QFileDialog::getSaveFileName(this, tr("Save As"), path, QString(), nullptr, fileDialogOptions());
        if (path.isEmpty())
            return false;
    }
    QString err;
    if (!ed->saveFile(path, &err)) {
        QMessageBox::warning(this, tr("Save"), tr("Cannot save %1:\n%2").arg(path, err));
        return false;
    }
    updateTabTitle(ed);
    m_tabs->setTabToolTip(m_tabs->indexOf(ed), ed->filePath());
    updateStatus();
    return true;
}

bool MainWindow::save()   { Editor *e = current(); return e && saveEditor(e, false); }
bool MainWindow::saveAs() { Editor *e = current(); return e && saveEditor(e, true); }

bool MainWindow::maybeSave(Editor *ed)
{
    if (!ed->isModified())
        return true;
    m_tabs->setCurrentWidget(ed);
    const auto r = QMessageBox::question(this, tr("Unsaved changes"),
        tr("Save changes to %1?").arg(ed->displayName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (r == QMessageBox::Save) return saveEditor(ed, false);
    return r == QMessageBox::Discard;
}

void MainWindow::closeTab(int index)
{
    Editor *ed = editorAt(index);
    if (!ed || !maybeSave(ed))
        return;
    m_tabs->removeTab(index);
    // Refresh the stats once the editor's memory has actually been freed.
    connect(ed, &QObject::destroyed, this, [this] {
#ifdef __GLIBC__
        malloc_trim(0); // hand freed memory back to the system
#endif
        updateTabMemStats();
    });
    ed->deleteLater();
    if (m_tabs->count() == 0)
        newTab();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (!maybeSave(editorAt(i))) {
            e->ignore();
            return;
        }
    }
    Prefs::settings().setValue("window/geometry", saveGeometry());
    Prefs::settings().sync();
    e->accept();
}

void MainWindow::updateTabTitle(Editor *ed)
{
    const int i = m_tabs->indexOf(ed);
    if (i < 0) return;
    m_tabs->setTabText(i, (ed->isModified() ? "*" : "") + ed->displayName());
    if (ed == current()) updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    Editor *e = current();
    if (!e) { setWindowTitle("BlitzPad"); return; }
    const QString name = e->filePath().isEmpty() ? e->displayName() : e->filePath();
    setWindowTitle((e->isModified() ? "*" : "") + name + " - BlitzPad");
}

void MainWindow::onCurrentTabChanged()
{
    Editor *e = current();
    if (!e) return;
    const QSignalBlocker b(m_wrapAction);
    m_wrapAction->setChecked(e->wrapMode() != QsciScintilla::WrapNone);
    e->setFocus();
    updateWindowTitle();
    updateStatus();
}

void MainWindow::updateStatus()
{
    Editor *e = current();
    if (!e) return;
    int line, col;
    e->getCursorPosition(&line, &col);
    m_posLabel->setText(tr("Ln %1, Col %2  |  %3 lines").arg(line + 1).arg(col + 1).arg(e->lines()));
    m_langLabel->setText(e->language());
    m_encLabel->setText(e->encodingName() + "  " + e->eolName());
    for (QAction *a : m_langGroup->actions())
        if (a->data().toString() == e->language()) { const QSignalBlocker b(m_langGroup); a->setChecked(true); }
}

// ---- Search -------------------------------------------------------------

QString MainWindow::selectedText() const
{
    Editor *e = current();
    return e ? e->selectedText() : QString();
}

void MainWindow::buildSearch()
{
    m_findDlg = new FindDialog(this);

    // Insert the Search menu after Edit, where Notepad++ has it.
    auto *search = new QMenu(tr("&Search"), this);
    menuBar()->insertMenu(menuBar()->actions().at(2), search);
    search->addAction(tr("&Find..."), QKeySequence::Find, this,
                      [this] { m_findDlg->showFind(selectedText()); });
    search->addAction(tr("&Replace..."), QKeySequence(Qt::CTRL | Qt::Key_H), this,
                      [this] { m_findDlg->showReplace(selectedText()); });
    search->addAction(tr("Find &Next"), QKeySequence(Qt::Key_F3), this, [this] { doFindNext(true); });
    search->addAction(tr("Find &Previous"), QKeySequence(Qt::SHIFT | Qt::Key_F3), this, [this] { doFindNext(false); });
    search->addSeparator();
    search->addAction(tr("&Go to Line..."), QKeySequence(Qt::CTRL | Qt::Key_G), this, [this] {
        Editor *e = current();
        if (!e) return;
        bool ok = false;
        int line, col;
        e->getCursorPosition(&line, &col);
        const int n = QInputDialog::getInt(this, tr("Go to Line"), tr("Line (1 - %1):").arg(e->lines()),
                                           line + 1, 1, e->lines(), 1, &ok);
        if (ok) { e->setCursorPosition(n - 1, 0); e->ensureLineVisible(n - 1); e->setFocus(); }
    });
    search->addAction(tr("Search &Results Panel"), QKeySequence(Qt::Key_F7), this, [this] {
        m_resultsDock->setVisible(!m_resultsDock->isVisible());
    });

    // Results panel for "Find All in All Open Tabs".
    m_results = new QTreeWidget(this);
    m_results->setHeaderHidden(true);
    m_results->setUniformRowHeights(true);
    m_results->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    connect(m_results, &QTreeWidget::itemActivated, this, &MainWindow::jumpToResult);
    connect(m_results, &QTreeWidget::itemClicked, this, &MainWindow::jumpToResult);
    m_resultsDock = new QDockWidget(tr("Search Results"), this);
    m_resultsDock->setObjectName("SearchResults");
    m_resultsDock->setWidget(m_results);
    addDockWidget(Qt::BottomDockWidgetArea, m_resultsDock);
    m_resultsDock->hide();

    connect(m_findDlg, &FindDialog::findNext, this, &MainWindow::doFindNext);
    connect(m_findDlg, &FindDialog::count, this, [this] {
        Editor *e = current();
        if (!e) return;
        const int n = Search::count(e, m_findDlg->options());
        if (n < 0) m_findDlg->setMessage(tr("Invalid regular expression."), true);
        else m_findDlg->setMessage(tr("Count: %n match(es).", nullptr, n));
    });
    connect(m_findDlg, &FindDialog::replaceOne, this, [this] {
        Editor *e = current();
        if (!e) return;
        const auto st = Search::replaceOne(e, m_findDlg->options());
        if (st == Search::Status::BadPattern) m_findDlg->setMessage(tr("Invalid regular expression."), true);
        else if (st == Search::Status::NotFound) m_findDlg->setMessage(tr("No more matches."), true);
        else m_findDlg->setMessage({});
    });
    connect(m_findDlg, &FindDialog::replaceAll, this, [this] {
        Editor *e = current();
        if (!e) return;
        const int n = Search::replaceAll(e, m_findDlg->options());
        if (n < 0) m_findDlg->setMessage(tr("Invalid regular expression."), true);
        else m_findDlg->setMessage(tr("Replace All: %n occurrence(s) replaced.", nullptr, n));
    });
    connect(m_findDlg, &FindDialog::replaceAllInTabs, this, &MainWindow::doReplaceAllInTabs);
    connect(m_findDlg, &FindDialog::findAllInTabs, this, &MainWindow::doFindAllInTabs);
}

void MainWindow::doFindNext(bool forward)
{
    Editor *e = current();
    if (!e) return;
    const SearchOptions o = m_findDlg->options();
    if (o.find.isEmpty()) { m_findDlg->showFind(selectedText()); return; }
    bool wrapped = false;
    const auto st = Search::findNext(e, o, forward, &wrapped);
    switch (st) {
    case Search::Status::BadPattern:
        m_findDlg->setMessage(tr("Invalid regular expression."), true);
        break;
    case Search::Status::NotFound:
        m_findDlg->setMessage(tr("Can't find \"%1\".").arg(o.find), true);
        statusBar()->showMessage(tr("Can't find \"%1\"").arg(o.find), 3000);
        break;
    case Search::Status::Found:
        m_findDlg->setMessage(wrapped ? tr("Reached the end, continued from the top.") : QString());
        if (wrapped) statusBar()->showMessage(tr("Search wrapped around"), 2000);
        break;
    }
}

void MainWindow::doReplaceAllInTabs()
{
    const SearchOptions o = m_findDlg->options();
    int total = 0, files = 0;
    for (int i = 0; i < m_tabs->count(); ++i) {
        const int n = Search::replaceAll(editorAt(i), o);
        if (n < 0) { m_findDlg->setMessage(tr("Invalid regular expression."), true); return; }
        total += n;
        if (n > 0) ++files;
    }
    m_findDlg->setMessage(tr("Replaced %1 occurrence(s) in %2 tab(s).").arg(total).arg(files));
}

void MainWindow::doFindAllInTabs()
{
    const SearchOptions o = m_findDlg->options();
    m_results->clear();
    int total = 0, files = 0;
    for (int t = 0; t < m_tabs->count(); ++t) {
        Editor *e = editorAt(t);
        QTreeWidgetItem *fileItem = nullptr;
        int hits = 0;
        long pos = 0, lastLine = -1;
        for (;;) {
            const long len = e->SendScintilla(QsciScintillaBase::SCI_GETLENGTH);
            if (pos > len) break;
            Match m;
            const auto st = Search::find(e, o, pos, len, &m);
            if (st == Search::Status::BadPattern) {
                m_findDlg->setMessage(tr("Invalid regular expression."), true);
                return;
            }
            if (st != Search::Status::Found) break;
            ++hits;
            const long line = e->SendScintilla(QsciScintillaBase::SCI_LINEFROMPOSITION, m.start);
            if (line != lastLine) { // one row per line, like Notepad++
                if (!fileItem) fileItem = new QTreeWidgetItem(m_results);
                QString text = e->text(int(line));
                text.chop(text.endsWith("\r\n") ? 2 : (text.endsWith('\n') || text.endsWith('\r')) ? 1 : 0);
                auto *it = new QTreeWidgetItem(fileItem, {tr("Line %1: %2").arg(line + 1).arg(text.left(300))});
                it->setData(0, Qt::UserRole, QVariant::fromValue<void *>(e));
                it->setData(0, Qt::UserRole + 1, qlonglong(m.start));
                it->setData(0, Qt::UserRole + 2, qlonglong(m.end));
                lastLine = line;
            }
            pos = m.end > m.start ? m.end : m.end + 1;
            // Skip to the next line once this line has a row; we only list lines.
            pos = qMax(pos, long(e->SendScintilla(QsciScintillaBase::SCI_GETLINEENDPOSITION, line)) + 1);
        }
        if (fileItem) {
            const QString name = e->filePath().isEmpty() ? e->displayName() : e->filePath();
            fileItem->setText(0, tr("%1 (%2 line(s))").arg(name).arg(fileItem->childCount()));
            fileItem->setExpanded(true);
            total += hits;
            ++files;
        }
    }
    auto *top = new QTreeWidgetItem({tr("Search \"%1\" (%2 matching line(s) in %3 tab(s))")
                                        .arg(o.find).arg(total).arg(files)});
    m_results->insertTopLevelItem(0, top);
    m_resultsDock->show();
    m_findDlg->setMessage(tr("%1 matching line(s) in %2 tab(s).").arg(total).arg(files));
}

void MainWindow::jumpToResult(QTreeWidgetItem *item)
{
    auto *e = static_cast<Editor *>(item->data(0, Qt::UserRole).value<void *>());
    if (!e) return;
    const int idx = m_tabs->indexOf(e); // tab may have been closed since
    if (idx < 0) return;
    m_tabs->setCurrentIndex(idx);
    const long s = item->data(0, Qt::UserRole + 1).toLongLong();
    const long en = item->data(0, Qt::UserRole + 2).toLongLong();
    e->SendScintilla(QsciScintillaBase::SCI_SETSEL, s, en);
    e->SendScintilla(QsciScintillaBase::SCI_SCROLLRANGE, en, s);
    e->setFocus();
}

QFileDialog::Options MainWindow::fileDialogOptions() const
{
    if (Prefs::get("view/useNativeFileDialogs", false).toBool())
        return {};
    return QFileDialog::DontUseNativeDialog;
}

// ---- About, stats, Save Compressed -----------------------------------------

void MainWindow::buildAbout()
{
    // A plain action on the menu bar itself, last in the row: one click opens it.
    m_aboutAction = menuBar()->addAction(tr("&About"));
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::showAbout()
{
    // Laid out as Jesse specified: title, 64x64 icon, version and build time,
    // author, motto, then an OKAY button.
    QDialog dlg(this);
    dlg.setWindowTitle(tr("About BlitzPad"));
    auto *lay = new QVBoxLayout(&dlg);
    lay->setSpacing(10);
    lay->setContentsMargins(28, 22, 28, 18);

    auto centred = [&](const QString &html) {
        auto *l = new QLabel(html, &dlg);
        l->setAlignment(Qt::AlignCenter);
        l->setTextFormat(Qt::RichText);
        l->setTextInteractionFlags(Qt::TextBrowserInteraction);
        l->setOpenExternalLinks(true);
        lay->addWidget(l);
        return l;
    };
    centred(QStringLiteral("<b style=\"font-size:15pt\">-- blitzpad --</b>"));

    auto *icon = new QLabel(&dlg);
    icon->setPixmap(QIcon(QStringLiteral(":/icons/blitzpad-64.png")).pixmap(64, 64));
    icon->setAlignment(Qt::AlignCenter);
    lay->addWidget(icon);

    centred(QStringLiteral("Version %1 (%2)<br>By JK Benedict "
                           "(<a href=\"https://github.com/jkbenedict\">https://github.com/jkbenedict</a>)")
                .arg(QStringLiteral(BLITZPAD_VERSION), QStringLiteral(BLITZPAD_BUILD_DATE)));
    centred(QStringLiteral("<i>\"I was tired of a slow TextEditor, so I worked fast for faster work, later!\"</i>"));

    auto *ok = new QPushButton(tr("OKAY"), &dlg);
    ok->setDefault(true);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    auto *row = new QHBoxLayout;
    row->addStretch();
    row->addWidget(ok);
    row->addStretch();
    lay->addSpacing(4);
    lay->addLayout(row);
    dlg.setFixedSize(dlg.sizeHint());
    dlg.exec(); // modal: OKAY returns to editing
    if (Editor *e = current()) e->setFocus();
}

static double residentMegabytes()
{
    // Second field of /proc/self/statm is resident pages.
    QFile f("/proc/self/statm");
    if (!f.open(QIODevice::ReadOnly))
        return -1;
    const QList<QByteArray> parts = f.readAll().split(' ');
    if (parts.size() < 2)
        return -1;
    return parts.at(1).toDouble() * double(sysconf(_SC_PAGESIZE)) / (1024.0 * 1024.0);
}

void MainWindow::updateTabMemStats()
{
    if (!m_statsLabel) return;
    const int tabs = m_tabs->count();
    const double mb = residentMegabytes();
    const QString mem = mb < 0 ? QStringLiteral("?") : QString::number(qRound(mb));
    m_statsLabel->setText(tr("%1 %2 | %3 MEM").arg(tabs).arg(tabs == 1 ? tr("Tab") : tr("Tabs")).arg(mem));
    m_statsLabel->setToolTip(tr("%1 open tab(s). BlitzPad is using %2 MB of RAM.\n"
                                "Updates when a tab opens or closes, and every 60 seconds.")
                                 .arg(tabs).arg(mb, 0, 'f', 1));
}

void MainWindow::backToFileMenu()
{
    // Cancel anywhere in Save Compressed drops you back into the File menu.
    QTimer::singleShot(0, this, [this] {
        menuBar()->setActiveAction(m_fileMenu->menuAction());
    });
}

// Asks for a password twice. Returns false if the user cancels.
static bool askPassword(QWidget *parent, const QString &fileName, QString *out)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(QObject::tr("Set Password"));
    auto *pw1 = new QLineEdit(&dlg);
    auto *pw2 = new QLineEdit(&dlg);
    pw1->setEchoMode(QLineEdit::Password);
    pw2->setEchoMode(QLineEdit::Password);
    auto *hint = new QLabel(&dlg);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    auto *form = new QFormLayout(&dlg);
    form->addRow(new QLabel(QObject::tr("Password for %1:").arg(fileName), &dlg));
    form->addRow(QObject::tr("Password:"), pw1);
    form->addRow(QObject::tr("Confirm:"), pw2);
    form->addRow(hint);
    form->addRow(buttons);
    QPushButton *ok = buttons->button(QDialogButtonBox::Ok);
    auto check = [=] {
        const bool match = !pw1->text().isEmpty() && pw1->text() == pw2->text();
        ok->setEnabled(match);
        hint->setText(pw2->text().isEmpty() || match ? QString() : QObject::tr("Passwords don't match."));
    };
    QObject::connect(pw1, &QLineEdit::textChanged, &dlg, check);
    QObject::connect(pw2, &QLineEdit::textChanged, &dlg, check);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    check();
    if (dlg.exec() != QDialog::Accepted)
        return false;
    *out = pw1->text();
    return true;
}

void MainWindow::saveCompressed()
{
    Editor *ed = current();
    if (!ed) return;
    if (Compress::findSevenZip().isEmpty()) {
        QMessageBox::warning(this, tr("Save Compressed"),
            tr("Save Compressed needs 7-Zip, which isn't installed.\n\n"
               "Zorin / Ubuntu / Debian: sudo apt install p7zip-full\n"
               "Fedora: sudo dnf install p7zip p7zip-plugins\n"
               "Arch: sudo pacman -S 7zip\n\n"
               "Or run build.sh again and let it install the missing packages."));
        return;
    }

    // 1. Where, and which format.
    QStringList filters;
    for (const auto &f : Compress::formats())
        filters << QStringLiteral("%1%2 (*%3)").arg(f.label,
                                                  f.supportsPassword ? QString() : tr(", no password"),
                                                  f.extension);
    QString inner = ed->filePath().isEmpty() ? QStringLiteral("untitled.txt") : QFileInfo(ed->filePath()).fileName();
    const QString dir = ed->filePath().isEmpty() ? QString() : QFileInfo(ed->filePath()).absolutePath();

    QFileDialog dlg(this, tr("Save Compressed"), dir);
    dlg.setOptions(fileDialogOptions());
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setNameFilters(filters);
    const int lastFmt = qBound(0, Prefs::settings().value("compress/lastFormat", 0).toInt(), int(filters.size()) - 1);
    dlg.selectNameFilter(filters.at(lastFmt));
    dlg.selectFile(inner + Compress::formats().at(lastFmt).extension);
    // Keep the suggested file name's extension in step with the chosen format.
    connect(&dlg, &QFileDialog::filterSelected, &dlg, [&](const QString &flt) {
        const int i = filters.indexOf(flt);
        if (i >= 0) dlg.selectFile(inner + Compress::formats().at(i).extension);
    });
    if (dlg.exec() != QDialog::Accepted || dlg.selectedFiles().isEmpty()) {
        backToFileMenu();
        return;
    }
    const int fmtIndex = qMax(0, int(filters.indexOf(dlg.selectedNameFilter())));
    const Compress::FormatInfo fi = Compress::formats().at(fmtIndex);
    Prefs::settings().setValue("compress/lastFormat", fmtIndex);
    QString path = dlg.selectedFiles().first();
    if (!path.endsWith(fi.extension, Qt::CaseInsensitive)) {
        path += fi.extension;
        if (QFileInfo::exists(path)
            && QMessageBox::question(this, tr("Save Compressed"), tr("%1 already exists. Replace it?").arg(path))
                   != QMessageBox::Yes) {
            backToFileMenu();
            return;
        }
    }

    // 2. Password, for formats that support one.
    QString password;
    if (fi.supportsPassword) {
        const auto r = QMessageBox::question(this, tr("Save Compressed"),
            tr("Protect %1 with a password?").arg(QFileInfo(path).fileName()),
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);
        if (r == QMessageBox::Cancel) { backToFileMenu(); return; }
        if (r == QMessageBox::Yes && !askPassword(this, QFileInfo(path).fileName(), &password)) {
            backToFileMenu();
            return;
        }
    }

    // 3. Write it.
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString err;
    const bool ok = Compress::save(fi.format, path, inner, ed->contentBytes(), password, &err);
    QApplication::restoreOverrideCursor();
    if (!ok) {
        QMessageBox::warning(this, tr("Save Compressed"), tr("Could not save %1:\n%2").arg(path, err));
        return;
    }
    statusBar()->showMessage(tr("Saved compressed copy: %1%2")
                                 .arg(path, password.isEmpty() ? QString() : tr(" (password protected)")), 6000);
}

// ---- Options menu, key bindings, reloading preferences ---------------------

void MainWindow::buildOptions()
{
    // Options sits just before About, so About stays last. Alt+O opens it.
    auto *opt = new QMenu(tr("&Options"), this);
    menuBar()->insertMenu(m_aboutAction, opt);
    opt->addAction(tr("&Global Colour Preferences..."), this, [this] {
        GlobalColoursDialog dlg(this);
        connect(&dlg, &GlobalColoursDialog::applied, this, &MainWindow::reapplyColours);
        dlg.exec();
    });
    opt->addAction(tr("G&rammar Colour Overrides..."), this, [this] {
        Editor *e = current();
        GrammarColoursDialog dlg(e ? e->language() : QStringLiteral("C++"), this);
        connect(&dlg, &GrammarColoursDialog::applied, this, &MainWindow::reapplyColours);
        dlg.exec();
    });
    opt->addAction(tr("Create File &Format..."), this, [this] { createFileFormat(QString(), nullptr); });
    opt->addSeparator();
    opt->addAction(tr("&Edit preferences.ini"), this, [this] {
        Prefs::settings().sync();
        openFile(Prefs::filePath());
        statusBar()->showMessage(tr("Edit and save, then use Options > Reload Preferences."), 8000);
    });
    opt->addAction(tr("Re&load Preferences"), this, &MainWindow::reloadPreferences);
}

void MainWindow::reapplyColours()
{
    for (int i = 0; i < m_tabs->count(); ++i)
        Theme::apply(editorAt(i), editorAt(i)->language());
}

static QString shortcutKey(const QString &menu, const QString &action)
{
    auto clean = [](QString t) {
        t.remove('&');
        t.remove(QStringLiteral("..."));
        t = t.toLower();
        t.replace(QRegularExpression("[^a-z0-9+]+"), "-");
        return t.remove(QRegularExpression("^-+|-+$"));
    };
    return "shortcuts/" + clean(menu) + "." + clean(action);
}

// Every menu command gets a line in [shortcuts]; edit it there to rebind.
// Several keys are separated by "; ". An empty value means no shortcut.
void MainWindow::applyShortcuts()
{
    QSettings &s = Prefs::settings();
    for (QAction *top : menuBar()->actions()) {
        QMenu *m = top->menu();
        if (!m) continue;
        for (QAction *a : m->actions()) {
            if (a->isSeparator() || a->menu() || a->text().isEmpty()) continue;
            const QString key = shortcutKey(top->text(), a->text());
            if (s.contains(key)) {
                QList<QKeySequence> seqs;
                for (const QString &part : s.value(key).toString().split(';', Qt::SkipEmptyParts)) {
                    const QKeySequence k = QKeySequence::fromString(part.trimmed(), QKeySequence::PortableText);
                    if (!k.isEmpty()) seqs << k;
                }
                a->setShortcuts(seqs);
            } else {
                QStringList parts;
                for (const QKeySequence &k : a->shortcuts())
                    parts << k.toString(QKeySequence::PortableText);
                s.setValue(key, parts.join("; "));
            }
        }
    }
}

void MainWindow::reloadPreferences()
{
    Prefs::settings().sync(); // pick up edits made to the file
    applyShortcuts();
    for (int i = 0; i < m_tabs->count(); ++i)
        editorAt(i)->reloadPreferences();
    statusBar()->showMessage(tr("Preferences reloaded from %1").arg(Prefs::filePath()), 5000);
}

// ---- Unknown formats and your own file formats ------------------------------

void MainWindow::rebuildLanguageMenu()
{
    for (QAction *a : m_langGroup->actions()) {
        m_langGroup->removeAction(a);
        delete a;
    }
    m_langMenu->clear();
    bool userSection = false;
    for (const Language &l : Languages::all()) {
        if (l.userDefined && !userSection) {
            m_langMenu->addSection(tr("Your formats"));
            userSection = true;
        }
        QAction *a = m_langMenu->addAction(l.name);
        a->setCheckable(true);
        a->setData(l.name);
        m_langGroup->addAction(a);
    }
    updateStatus();
}

// Opens Options > Create File Format. If `forEditor` is given, the new
// format is applied to it when saved.
bool MainWindow::createFileFormat(const QString &extension, Editor *forEditor)
{
    QPointer<Editor> target(forEditor);
    FileFormatDialog dlg(extension, this);
    if (dlg.exec() != QDialog::Accepted)
        return false;
    rebuildLanguageMenu();
    const QString name = dlg.savedName();
    // Re-colour tabs already using this format, then apply it where asked.
    for (int i = 0; i < m_tabs->count(); ++i)
        if (editorAt(i)->language() == name) editorAt(i)->setLanguage(name);
    if (target) {
        target->setLanguage(name);
        target->clearUnknownExtension();
    }
    updateStatus();
    statusBar()->showMessage(tr("Saved file format \"%1\" in %2").arg(name, UserLanguage::directory()), 6000);
    return true;
}

void MainWindow::askAboutUnknownFormat(Editor *ed)
{
    const QString ext = ed->unknownExtension();
    if (ext.isEmpty()) return;
    QSettings &s = Prefs::settings();
    const QStringList plain = s.value("file-types/plain-text-extensions").toString().split(' ', Qt::SkipEmptyParts);
    if (!s.value("file-types/ask-about-unknown-formats", true).toBool() || plain.contains(ext)) {
        ed->clearUnknownExtension();
        return;
    }
    m_tabs->setCurrentWidget(ed);

    QMessageBox box(this);
    box.setWindowTitle(tr("Unknown file format"));
    box.setIcon(QMessageBox::Question);
    box.setText(tr("BlitzPad doesn't recognise the format of <b>%1</b> (.%2 files).")
                    .arg(ed->displayName().toHtmlEscaped(), ext.toHtmlEscaped()));
    box.setInformativeText(tr("Open it as plain text, or create a file format for .%1 files "
                              "with its own grammar, symbols, folding and colours?").arg(ext));
    QPushButton *plainBtn = box.addButton(tr("Plain Text"), QMessageBox::AcceptRole);
    QPushButton *createBtn = box.addButton(tr("Create File Format..."), QMessageBox::ActionRole);
    box.addButton(QMessageBox::Cancel);
    box.setDefaultButton(plainBtn);
    auto *always = new QCheckBox(tr("Always open .%1 files as plain text").arg(ext), &box);
    box.setCheckBox(always);
    box.exec();

    QPointer<Editor> p(ed);
    if (box.clickedButton() == createBtn) {
        if (p && createFileFormat(ext, p))
            return;
    } else if (always->isChecked()) {
        QStringList list = plain;
        list << ext;
        s.setValue("file-types/plain-text-extensions", list.join(' '));
        s.sync();
    }
    // Plain Text, Cancel, or the format window was cancelled: plain old text.
    if (p) p->clearUnknownExtension();
}
