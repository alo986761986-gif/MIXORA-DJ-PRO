#pragma once
#include <QMainWindow>
#include <QStringList>
class DeckWidget;class QSlider;class QLabel;class QListWidget;class QLineEdit;
class MainWindow : public QMainWindow {
 Q_OBJECT
public:
 explicit MainWindow(QWidget* parent=nullptr);
private:
 DeckWidget *m_a=nullptr,*m_b=nullptr;
 QSlider *m_cross=nullptr,*m_master=nullptr;
 QLabel *m_status=nullptr;
 QListWidget* m_library=nullptr;
 QLineEdit* m_search=nullptr;
 QStringList m_tracks;
 void addFiles(const QStringList& files);
 void filterLibrary();
 void loadSelected(bool deckA);
 void updateMixer();
};
