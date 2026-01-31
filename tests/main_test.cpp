#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QString>
#include <QPoint>
#include <QVector>

// Include headers from the main application
#include "projectconfig.h"
#include "polygoncanvas.h"

// Test fixture for PolySeg tests
class PolySegTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up any common test data or objects
        argc = 1;
        argv = new char*[1];
        argv[0] = const_cast<char*>("test");
        app = new QCoreApplication(argc, argv);
    }

    void TearDown() override {
        delete app;
        delete[] argv;
    }

    int argc;
    char** argv;
    QCoreApplication* app;
};

// Basic test to ensure Google Test is working
TEST_F(PolySegTest, BasicTest) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_TRUE(true);
}

// Test ProjectConfig functionality
TEST_F(PolySegTest, ProjectConfigDefaultValues) {
    ProjectConfig config;

    // Test default values based on actual ProjectConfig implementation
    EXPECT_FALSE(config.GetProjectName().isEmpty());  // Should have default name
    EXPECT_FALSE(config.GetVersion().isEmpty());      // Should have version
    EXPECT_EQ(config.GetClasses().size(), 0);         // No classes by default
    EXPECT_EQ(config.GetTotalImages(), 0);            // No images by default
    EXPECT_EQ(config.GetLabeledImages(), 0);          // No labeled images by default
}

// Test basic Qt functionality
TEST_F(PolySegTest, QtStringOperations) {
    QString testString = "PolySeg Test";
    EXPECT_EQ(testString.length(), 12);
    EXPECT_TRUE(testString.contains("Test"));
    EXPECT_TRUE(testString.startsWith("PolySeg"));
}

// Test QPoint operations (used in polygon functionality)
TEST_F(PolySegTest, QPointOperations) {
    QPoint point1(10, 20);
    QPoint point2(30, 40);

    EXPECT_EQ(point1.x(), 10);
    EXPECT_EQ(point1.y(), 20);
    EXPECT_EQ(point2.x(), 30);
    EXPECT_EQ(point2.y(), 40);

    // Test vector operations
    QVector<QPoint> points;
    points.append(point1);
    points.append(point2);

    EXPECT_EQ(points.size(), 2);
    EXPECT_EQ(points[0], point1);
    EXPECT_EQ(points[1], point2);
}

// Test polygon-related calculations
TEST_F(PolySegTest, DistanceCalculation) {
    // This tests a basic distance calculation similar to what's used in PolygonCanvas
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

// Test coordinate normalization (used in annotation export)
TEST_F(PolySegTest, CoordinateNormalization) {
    // Test coordinate normalization functionality
    auto normalize = [](int coord, int max_size) -> double {
        return static_cast<double>(coord) / static_cast<double>(max_size);
    };

    int imageWidth = 800;
    int imageHeight = 600;

    // Test corner coordinates
    EXPECT_NEAR(normalize(0, imageWidth), 0.0, 0.001);
    EXPECT_NEAR(normalize(imageWidth, imageWidth), 1.0, 0.001);
    EXPECT_NEAR(normalize(400, imageWidth), 0.5, 0.001);

    EXPECT_NEAR(normalize(0, imageHeight), 0.0, 0.001);
    EXPECT_NEAR(normalize(imageHeight, imageHeight), 1.0, 0.001);
    EXPECT_NEAR(normalize(300, imageHeight), 0.5, 0.001);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}