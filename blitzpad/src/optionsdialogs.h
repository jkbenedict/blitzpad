#pragma once

#include <QDialog>
#include <QHash>
#include <QToolButton>
#include <QVariant>

class QComboBox;
class QTableWidget;
class QsciScintilla;

// A swatch button for one colour. An invalid colour means "use the default".
class ColourButton : public QToolButton {
    Q_OBJECT
public:
    ColourButton(const QColor &value, const QColor &fallback, const QString &title, QWidget *parent);
    QColor value() const { return m_value; }
    void setValue(const QColor &c);
    void setFallback(const QColor &c) { m_fallback = c; refresh(); }

signals:
    void changed(const QColor &c);

private:
    void refresh();
    QColor m_value, m_fallback;
    QString m_title;
};

// Changes are applied live (so you see them in open tabs straight away);
// Cancel puts everything back the way it was when the window opened.
class GlobalColoursDialog : public QDialog {
    Q_OBJECT
public:
    explicit GlobalColoursDialog(QWidget *parent = nullptr);
signals:
    void applied();
protected:
    void reject() override;
private:
    void rebuild();
    QTableWidget *m_table;
    QHash<QString, QVariant> m_snapshot;
};

class GrammarColoursDialog : public QDialog {
    Q_OBJECT
public:
    explicit GrammarColoursDialog(const QString &language, QWidget *parent = nullptr);
signals:
    void applied();
protected:
    void reject() override;
private:
    void rebuild();
    void refreshPreview();
    QComboBox *m_lang;
    QTableWidget *m_table;
    QsciScintilla *m_preview;
    QHash<QString, QVariant> m_snapshot;
};
