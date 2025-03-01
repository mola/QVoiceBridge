#include "frequencyspectrum.h"

#include <math.h>

#include <QPainter>
#include <QPainterPath>
#include <QTimer>

#include <QDebug>

FrequencySpectrum::FrequencySpectrum(QWidget *parent):
    QWidget(parent)
    , numSamples(0)
    , maxPower(1)
    , redrawTimer(new QTimer(this))
    , frequencyColor(Qt::red)
    , threshold(10.0)
{
}

void  FrequencySpectrum::frequenciesChanged(const double *frequencies, const int numSamples)
{
    this->frequencies = frequencies;
    this->numSamples  = numSamples;
    update();
}

void  FrequencySpectrum::setThreshold(double thresholdValue)
{
    // Ensure the threshold is within the range of the frequency amplitudes
    if (thresholdValue < 0.0)
    {
        thresholdValue = 0.0;
    }
    else if (thresholdValue > maxPower)
    {
        thresholdValue = maxPower;
    }

    threshold = thresholdValue;
    // trigger a repaint so the new threshold is visible immediately
    update();
}

double  FrequencySpectrum::getThreshold() const
{
    return threshold;
}

FrequencySpectrum::~FrequencySpectrum()
{
}

void  FrequencySpectrum::reset()
{
    maxPower = 0.0;
    update();
}

void  FrequencySpectrum::redrawTimerExpired()
{
    update();
}

void  FrequencySpectrum::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    if (numSamples == 0) { return; }

    QPainter  painter(this);

    painter.fillRect(rect(), Qt::white);
    painter.setPen(frequencyColor);

    QRect   bar       = rect();
    int     maxHeight = bar.height();  // height of drawing area
    QPoint  lastPoint(0, maxHeight);

    painter.drawPoint(lastPoint);

    int     pixelsWide = bar.width();
    double  xStep      = static_cast<double>(pixelsWide) / numSamples;

    // Update maxPower based on the current samples
    for (int i = 0; i < static_cast<int>(numSamples); i++)
    {
        maxPower = std::max(maxPower, frequencies[i]);
    }

    // Draw the frequency spectrum
    for (int i = 0; i < static_cast<int>(numSamples); i++)
    {
        int     x = static_cast<int>(i * xStep);
        int     y = maxHeight - static_cast<int>((frequencies[i] / maxPower) * maxHeight);
        QPoint  currentPoint(x, y);

        painter.drawLine(lastPoint, currentPoint);
        lastPoint = currentPoint;
    }

    // Draw the threshold line
    QPen  thresholdPen(Qt::darkGreen);

    thresholdPen.setStyle(Qt::DashLine);
    thresholdPen.setWidth(2);
    painter.setPen(thresholdPen);

    int  thresholdY = maxHeight;

    if (maxPower > 0)
    {
        // Ensure the threshold is within the range of the frequency amplitudes
        thresholdY = maxHeight - static_cast<int>((threshold / maxPower) * maxHeight);
    }

    painter.drawLine(0, thresholdY, bar.width(), thresholdY);
}
