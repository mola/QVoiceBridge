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
    , threshold(0.5)
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
    threshold = thresholdValue;
    update();  // trigger a repaint so the new threshold is visible immediately
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

    QRect  bar       = rect();
    int    maxHeight = bar.height();  // height of drawing area
    // Start drawing the spectrum from the left-bottom corner.
    QPoint  lastPoint(0, maxHeight);

    // Draw an initial point
    painter.drawPoint(lastPoint);

    int     pixelsWide = bar.width();
    double  xStep      = static_cast<double>(pixelsWide) / numSamples;

    // Since maxPower might change based on incoming data,
    // update maxPower based on the current samples.
    for (int i = 0; i < static_cast<int>(numSamples); i++)
    {
        maxPower = std::max(maxPower, frequencies[i]);
    }

    // Draw the frequency spectrum without averaging.
    for (int i = 0; i < static_cast<int>(numSamples); i++)
    {
        // Calculate the x coordinate for the current frequency sample.
        int  x = static_cast<int>(i * xStep);

        // Map the frequency amplitude to a y coordinate.
        // The higher the amplitude, the lower the y coordinate.
        int     y = maxHeight - static_cast<int>((frequencies[i] / maxPower) * maxHeight);
        QPoint  currentPoint(x, y);

        painter.drawLine(lastPoint, currentPoint);
        lastPoint = currentPoint;
    }

    // After drawing frequency data, add a horizontal green threshold line.
    QPen  thresholdPen(Qt::darkGreen);

    thresholdPen.setStyle(Qt::DashLine);
    thresholdPen.setWidth(2);
    painter.setPen(thresholdPen);

    // Compute the y coordinate for the threshold line.
    // If maxPower is not 0, scale accordingly; otherwise, place at the bottom.
    int  thresholdY = maxHeight;

    if (maxPower > 0)
    {
        thresholdY = maxHeight - static_cast<int>((threshold / maxPower) * maxHeight);
    }

    painter.drawLine(0, thresholdY, bar.width(), thresholdY);
}
