#pragma once
#include <QWidget>
#include <QColor>
#include <QVector>
class WaveformWidget : public QWidget {
 Q_OBJECT
public:
 explicit WaveformWidget(QWidget *parent=nullptr);
 void setPeaks(const QVector<float>& peaks);
 void setProgress(double position);
 void setColor(const QColor& color);
signals:
 void seekRequested(double fraction);
protected:
 void paintEvent(QPaintEvent*) override;
 void mousePressEvent(QMouseEvent*) override;
private:
 QVector<float> m_peaks;
 double m_progress=0;
 QColor m_color=QColor("#38c5fb");
};
