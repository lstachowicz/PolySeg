#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QString>
#include <QPoint>
#include <QVector>
#include <cmath>

#include "projectconfig.h"

class ProjectConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        argc_ = 1;
        argv_ = new char*[1];
        argv_[0] = const_cast<char*>("test");
        app_ = new QCoreApplication(argc_, argv_);
    }

    void TearDown() override {
        delete app_;
        delete[] argv_;
    }

    int argc_;
    char** argv_;
    QCoreApplication* app_;
};

TEST_F(ProjectConfigTest, BasicTest) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_TRUE(true);
}

TEST_F(ProjectConfigTest, ProjectConfigDefaultValues) {
    ProjectConfig config;

    EXPECT_FALSE(config.GetProjectName().isEmpty());
    EXPECT_FALSE(config.GetVersion().isEmpty());
    EXPECT_EQ(config.GetClasses().size(), 0);
    EXPECT_EQ(config.GetTotalImages(), 0);
    EXPECT_EQ(config.GetLabeledImages(), 0);
}

TEST_F(ProjectConfigTest, QtStringOperations) {
    QString testString = "PolySeg Test";
    EXPECT_EQ(testString.length(), 12);
    EXPECT_TRUE(testString.contains("Test"));
    EXPECT_TRUE(testString.startsWith("PolySeg"));
}

TEST_F(ProjectConfigTest, QPointOperations) {
    QPoint point1(10, 20);
    QPoint point2(30, 40);

    EXPECT_EQ(point1.x(), 10);
    EXPECT_EQ(point1.y(), 20);
    EXPECT_EQ(point2.x(), 30);
    EXPECT_EQ(point2.y(), 40);

    QVector<QPoint> points;
    points.append(point1);
    points.append(point2);

    EXPECT_EQ(points.size(), 2);
    EXPECT_EQ(points[0], point1);
    EXPECT_EQ(points[1], point2);
}

TEST_F(ProjectConfigTest, DistanceCalculation) {
    auto distance = [](const QPoint& p1, const QPoint& p2) -> double {
        int dx = p2.x() - p1.x();
        int dy = p2.y() - p1.y();
        return std::sqrt(dx * dx + dy * dy);
    };

    QPoint p1(0, 0);
    QPoint p2(3, 4);

    double dist = distance(p1, p2);
    EXPECT_NEAR(dist, 5.0, 0.001);  // 3-4-5 triangle
}

TEST_F(ProjectConfigTest, CoordinateNormalization) {
    auto normalize = [](int coord, int max_size) -> double {
        return static_cast<double>(coord) / static_cast<double>(max_size);
    };

    int imageWidth = 800;
    int imageHeight = 600;

    EXPECT_NEAR(normalize(0, imageWidth), 0.0, 0.001);
    EXPECT_NEAR(normalize(imageWidth, imageWidth), 1.0, 0.001);
    EXPECT_NEAR(normalize(400, imageWidth), 0.5, 0.001);

    EXPECT_NEAR(normalize(0, imageHeight), 0.0, 0.001);
    EXPECT_NEAR(normalize(imageHeight, imageHeight), 1.0, 0.001);
    EXPECT_NEAR(normalize(300, imageHeight), 0.5, 0.001);
}
