#include "mainwindow.h"
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QFont>
#include <QInputDialog>
#include <QGraphicsLineItem>
#include <QMouseEvent>
#include <QQueue>
#include <QMessageBox>
#include <QDebug>
#include <cmath> // Để tính khoảng cách Euclid

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    scene(new QGraphicsScene(this)),
    view(new QGraphicsView(scene, this)),
    vertexCounter('A')
{
    // Cài đặt cửa sổ chính
    setWindowTitle("Bellman-Ford Visualization");
    resize(800, 600);
    setCentralWidget(view);

    // Kích hoạt khử răng cưa
    view->setRenderHint(QPainter::Antialiasing);

    // Thêm ảnh bản đồ vào scene
    QPixmap mapPixmap(":/resources/map.png");
    if (mapPixmap.isNull()) {
        qDebug("Không tìm thấy ảnh bản đồ, kiểm tra đường dẫn!");
    } else {
        QGraphicsPixmapItem *mapItem = scene->addPixmap(mapPixmap);
        mapItem->setZValue(-1);
    }

    // Tạo nút thêm cạnh
    addEdgeButton = new QPushButton("Thêm cạnh", this);
    addEdgeButton->setGeometry(10, 50, 150, 30);
    addEdgeButton->setStyleSheet("background-color: red");
    connect(addEdgeButton, &QPushButton::clicked, this, &MainWindow::onAddEdge);

    // Tạo nút "Tìm đường đi ngắn nhất"
    QPushButton* findShortestPathButton = new QPushButton("Tìm đường đi ngắn nhất", this);
    findShortestPathButton->setGeometry(10, 100, 150, 30);
    findShortestPathButton->setStyleSheet("background-color: red");
    connect(findShortestPathButton, &QPushButton::clicked, this, &MainWindow::onFindShortestPath);

    // Tạo nút "Đảo dấu trọng số"
    QPushButton* toggleWeightSignButton = new QPushButton("Đảo dấu trọng số", this);
    toggleWeightSignButton->setGeometry(10, 150, 150, 30);
    toggleWeightSignButton->setStyleSheet("background-color: red");
    connect(toggleWeightSignButton, &QPushButton::clicked, this, &MainWindow::onToggleWeightSign);
}

MainWindow::~MainWindow() {}

// Hàm tính khoảng cách Euclid
double MainWindow::calculateEuclideanDistance(const QPointF& p1, const QPointF& p2) {
    return std::sqrt(std::pow(p1.x() - p2.x(), 2) + std::pow(p1.y() - p2.y(), 2));
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    QPointF sceneMapped = view->mapToScene(event->pos());

    // Thêm đỉnh vào đồ thị
    QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(sceneMapped.x() - 5, sceneMapped.y() - 5, 10, 10);
    ellipse->setBrush(Qt::red);  // Đặt màu đỏ cho chấm

    QChar vertexName = vertexCounter;  // Tạo tên đỉnh (A, B, C,...)
    vertexCounter = QChar(vertexCounter.toLatin1() + 1);  // Tăng giá trị của vertexCounter

    // Lưu lại thông tin đỉnh
    Vertex vertex = { sceneMapped, vertexName, ellipse };
    verticesMap[vertexName] = vertex;

    QGraphicsTextItem* label = new QGraphicsTextItem(vertexName);
    label->setPos(sceneMapped.x() + 10, sceneMapped.y() + 10);  // Đặt tên đỉnh gần vị trí của chấm

    scene->addItem(ellipse);
    scene->addItem(label);
}

void MainWindow::onAddEdge() {
    if (verticesMap.size() < 2) {
        qDebug("Cần ít nhất 2 đỉnh để thêm cạnh.");
        return;
    }

    // Hiển thị hộp thoại để chọn hai đỉnh
    bool ok;
    QString edgeData = QInputDialog::getText(this, "Thêm cạnh", "Nhập hai đỉnh (ví dụ: A B):", QLineEdit::Normal, "", &ok);

    if (!ok || edgeData.isEmpty())
        return;

    // Phân tích dữ liệu nhập
    QStringList parts = edgeData.split(" ");
    if (parts.size() != 2) {
        qDebug("Dữ liệu nhập không hợp lệ. Vui lòng nhập theo định dạng: <đỉnh nguồn> <đỉnh đích>");
        return;
    }

    QChar from = parts[0].at(0).toUpper();
    QChar to = parts[1].at(0).toUpper();

    if (!verticesMap.contains(from) || !verticesMap.contains(to)) {
        qDebug("Một hoặc cả hai đỉnh không tồn tại.");
        return;
    }

    // Tính khoảng cách Euclid làm trọng số
    Vertex fromVertex = verticesMap[from];
    Vertex toVertex = verticesMap[to];
    double distance = calculateEuclideanDistance(fromVertex.position, toVertex.position);
    int weight = static_cast<int>(distance);

    // Vẽ cạnh lên scene
    QGraphicsLineItem* line = scene->addLine(QLineF(fromVertex.position, toVertex.position), QPen(Qt::blue, 5));
    QPointF midpoint = (fromVertex.position + toVertex.position) / 2;

    // Hiển thị trọng số tại trung điểm
    QGraphicsTextItem* textItem = new QGraphicsTextItem(QString::number(weight));
    textItem->setPos(midpoint);
    textItem->setDefaultTextColor(Qt::black);
    scene->addItem(textItem);

    // Lưu thông tin cạnh và trọng số
    edges.append({from, to, weight});
    edges.append({to, from, weight});
}

void MainWindow::onFindShortestPath() {
    bool ok;
    QString source = QInputDialog::getText(this, "Nhập đỉnh nguồn", "Nhập đỉnh nguồn:", QLineEdit::Normal, "", &ok);
    QString target = QInputDialog::getText(this, "Nhập đỉnh đích", "Nhập đỉnh đích:", QLineEdit::Normal, "", &ok);

    if (!ok || source.isEmpty() || target.isEmpty()) return;

    // Khởi tạo các giá trị cho thuật toán Bellman-Ford
    QMap<QChar, int> distance;  // Khoảng cách từ nguồn
    QMap<QChar, QChar> previous;  // Lưu lại đỉnh đi qua

    for (const QChar& vertex : verticesMap.keys()) {
        distance[vertex] = INT_MAX;  // Đặt khoảng cách ban đầu là vô hạn
        previous[vertex] = QChar();  // Chưa có đỉnh đi qua
    }
    distance[source.at(0)] = 0;  // Đặt khoảng cách từ nguồn về 0

    // Thuật toán Bellman-Ford
    for (int i = 0; i < verticesMap.size() - 1; ++i) {
        for (const Edge& edge : edges) {
            QChar u = edge.from;
            QChar v = edge.to;
            int weight = edge.weight;

            // Cập nhật khoảng cách
            if (distance[u] != INT_MAX && distance[u] + weight < distance[v]) {
                distance[v] = distance[u] + weight;
                previous[v] = u;
            }
        }
    }

    // Kiểm tra chu trình âm sau khi thuật toán Bellman-Ford kết thúc
    bool hasNegativeCycle = false;
    for (const Edge& edge : edges) {
        QChar u = edge.from;
        QChar v = edge.to;
        int weight = edge.weight;

        // Nếu có thể cập nhật được trọng số, chứng tỏ có chu trình âm
        if (distance[u] != INT_MAX && distance[u] + weight < distance[v]) {
            // Tìm chu trình âm
            hasNegativeCycle = true;
            break;
        }
    }

    if (hasNegativeCycle) {
        // Kiểm tra và tìm chu trình âm bắt đầu từ nguồn
        QMap<QChar, bool> inCycle;
        QChar current = source.at(0);

        // Dùng thuật toán tìm chu trình âm
        QList<QChar> cycle;
        QChar cycleStart = current;

        // Lần theo chu trình âm
        while (previous.contains(current) && !inCycle.contains(current)) {
            inCycle[current] = true;
            cycle.append(current);
            current = previous[current];
        }

        // Nếu phát hiện chu trình, thông báo lỗi
        if (inCycle.contains(current)) {
            QMessageBox::critical(this, "Lỗi", "Đồ thị chứa chu trình âm.");
        }
        return;  // Dừng lại và không tiếp tục thực hiện
    }

    // Nếu không có chu trình âm, tiếp tục thực hiện tìm đường đi ngắn nhất
    QList<QChar> path;
    QChar current = target.at(0);
    int totalWeight = 0;

    while (current != QChar()) {
        path.prepend(current);
        current = previous[current];
    }

    // Tính tổng trọng số
    for (int i = 0; i < path.size() - 1; ++i) {
        QChar u = path[i];
        QChar v = path[i + 1];
        for (const Edge& edge : edges) {
            if (edge.from == u && edge.to == v) {
                totalWeight += edge.weight;
                break;
            }
        }
    }

    // Chuyển QList<QChar> thành QStringList
    QStringList pathStringList;
    for (const QChar& c : path) {
        pathStringList.append(QString(c));
    }

    QString result = "Đường đi ngắn nhất từ " + source + " đến " + target + ": " + pathStringList.join(" -> ");
    result += "\nTổng trọng số: " + QString::number(totalWeight);

    QMessageBox::information(this, "Kết quả", result);

    // Tô màu các đỉnh và cạnh trên đường đi ngắn nhất
    for (int i = 0; i < path.size(); ++i) {
        QChar vertex = path[i];
        Vertex vertexInfo = verticesMap[vertex];
        vertexInfo.ellipseItem->setBrush(Qt::green);
        coloredVertices.append(vertexInfo.ellipseItem);
    }

    for (int i = 0; i < path.size() - 1; ++i) {
        QChar u = path[i];
        QChar v = path[i + 1];
        QPointF uPos = verticesMap[u].position;
        QPointF vPos = verticesMap[v].position;
        QGraphicsLineItem* line = new QGraphicsLineItem(QLineF(uPos, vPos));
        line->setPen(QPen(Qt::green, 3));
        scene->addItem(line);
        coloredEdges.append(line);
    }
}

void MainWindow::onToggleWeightSign()
{
    if (edges.isEmpty()) {
        QMessageBox::information(this, "Thông báo", "Không có cạnh nào để đảo dấu.");
        return;
    }

    // Hiển thị hộp thoại để chọn hai đỉnh
    bool ok;
    QString edgeData = QInputDialog::getText(
        this, "Đảo dấu trọng số",
        "Nhập hai đỉnh của cạnh cần đổi dấu (ví dụ: A B):",
        QLineEdit::Normal, "", &ok);

    if (!ok || edgeData.isEmpty())
        return;

    // Phân tích dữ liệu nhập
    QStringList parts = edgeData.split(" ");
    if (parts.size() != 2) {
        QMessageBox::warning(this, "Lỗi", "Dữ liệu nhập không hợp lệ. Vui lòng nhập theo định dạng: <đỉnh nguồn> <đỉnh đích>.");
        return;
    }

    QChar from = parts[0].at(0).toUpper();
    QChar to = parts[1].at(0).toUpper();

    // Tìm và đổi dấu trọng số của cạnh
    bool edgeFound = false;
    for (Edge& edge : edges) {
        if ((edge.from == from && edge.to == to) || (edge.from == to && edge.to == from)) {
            edge.weight = -edge.weight;  // Đảo dấu trọng số
            edgeFound = true;

            // Cập nhật hiển thị trọng số trên scene
            for (QGraphicsItem* item : scene->items()) {
                if (auto textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
                    QString text = textItem->toPlainText();
                    bool ok;
                    int weight = text.toInt(&ok);
                    if (ok && weight == -edge.weight) {
                        textItem->setPlainText(QString::number(edge.weight));
                        break;
                    }
                }
            }
            break;
        }
    }

    if (!edgeFound) {
        QMessageBox::warning(this, "Lỗi", "Cạnh không tồn tại trong đồ thị.");
    } else {
        QMessageBox::information(this, "Thành công", "Đã đảo dấu trọng số của cạnh " + QString(from) + " -> " + QString(to) + ".");
    }
}
