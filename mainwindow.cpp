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

    // Khai báo thêm nút "Tìm đường đi ngắn nhất"
    QPushButton* findShortestPathButton = new QPushButton("Tìm đường đi ngắn nhất", this);
    findShortestPathButton->setGeometry(10, 100, 150, 30);  // Đặt vị trí và kích thước của nút
    findShortestPathButton->setStyleSheet("background-color: red");
    connect(findShortestPathButton, &QPushButton::clicked, this, &MainWindow::onFindShortestPath);
}

MainWindow::~MainWindow() {}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    QPointF sceneMapped = view->mapToScene(event->pos());

    // Thêm đỉnh vào đồ thị
    QGraphicsEllipseItem *ellipse = new QGraphicsEllipseItem(sceneMapped.x() - 5, sceneMapped.y() - 5, 10, 10);
    ellipse->setBrush(Qt::red);  // Đặt màu đỏ cho chấm

    QChar vertexName = vertexCounter;  // Tạo tên đỉnh (A, B, C,...)
    vertexCounter = QChar(vertexCounter.toLatin1() + 1);  // Tăng giá trị của vertexCounter

    // Lưu lại thông tin đỉnh (Sử dụng QChar thay vì QString)
    Vertex vertex = { sceneMapped, vertexName, ellipse };  // Lưu lại thông tin đỉnh
    verticesMap[vertexName] = vertex;  // Dùng QChar để truy cập map

    QGraphicsTextItem* label = new QGraphicsTextItem(vertexName);
    label->setPos(sceneMapped.x() + 10, sceneMapped.y() + 10);  // Đặt tên đỉnh gần vị trí của chấm

    scene->addItem(ellipse);
    scene->addItem(label);
}
void MainWindow::onAddEdge()
{
    if (verticesMap.size() < 2) {
        qDebug("Cần ít nhất 2 đỉnh để thêm cạnh.");
        return;
    }

    // Hiển thị hộp thoại nhập liệu
    bool ok;
    QString edgeData = QInputDialog::getText(
        this, "Thêm cạnh",
        "Nhập hai đỉnh và trọng số (ví dụ: A B -10):",  // Cho phép nhập trọng số âm
        QLineEdit::Normal, "", &ok);

    if (!ok || edgeData.isEmpty())
        return;

    // Phân tích dữ liệu nhập
    QStringList parts = edgeData.split(" ");
    if (parts.size() != 3) {
        qDebug("Dữ liệu nhập không hợp lệ. Vui lòng nhập theo định dạng: <đỉnh nguồn> <đỉnh đích> <trọng số>");
        return;
    }

    QChar from = parts[0].at(0).toUpper();
    QChar to = parts[1].at(0).toUpper();
    bool weightOk;
    int weight = parts[2].toInt(&weightOk);  // Trọng số có thể âm, sử dụng toInt để chuyển đổi

    if (!weightOk) {  // Kiểm tra tính hợp lệ của trọng số
        qDebug("Trọng số không hợp lệ. Vui lòng nhập một số hợp lệ.");
        return;
    }

    if (!verticesMap.contains(from) || !verticesMap.contains(to)) {
        qDebug("Một hoặc cả hai đỉnh không tồn tại.");
        return;
    }

    // Vẽ cạnh lên scene
    Vertex fromVertex = verticesMap[from];
    Vertex toVertex = verticesMap[to];

    // Vẽ cạnh từ "from" đến "to"
    QGraphicsLineItem* line = scene->addLine(QLineF(fromVertex.position, toVertex.position), QPen(Qt::blue, 5));

    // Vẽ cạnh từ "to" đến "from" nếu là cạnh vô hướng
    QGraphicsLineItem* reverseLine = scene->addLine(QLineF(toVertex.position, fromVertex.position), QPen(Qt::blue, 5));

    // Tính toán vị trí trung điểm của cạnh
    QPointF midpoint = (fromVertex.position + toVertex.position) / 2;

    // Tạo đối tượng văn bản để hiển thị trọng số tại trung điểm
    QGraphicsTextItem* textItem = new QGraphicsTextItem(QString::number(weight));
    textItem->setPos(midpoint);  // Đặt vị trí của văn bản ở trung điểm
    textItem->setDefaultTextColor(Qt::black);  // Đặt màu chữ là đen
    scene->addItem(textItem);

    // Lưu thông tin cạnh và trọng số
    edges.append({from, to, weight});  // Lưu thông tin cạnh
    edges.append({to, from, weight});  // Lưu cạnh ngược (vô hướng)
}

#include <cmath> // Để tính khoảng cách Euclid

void MainWindow::onFindShortestPath() {
    bool ok;
    QString source = QInputDialog::getText(this, "Nhập đỉnh nguồn", "Nhập đỉnh nguồn:", QLineEdit::Normal, "", &ok);
    QString target = QInputDialog::getText(this, "Nhập đỉnh đích", "Nhập đỉnh đích:", QLineEdit::Normal, "", &ok);

    if (!ok || source.isEmpty() || target.isEmpty()) return;

    // Xóa các đỉnh và cạnh đã tô màu cũ
    for (QGraphicsEllipseItem* vertexItem : coloredVertices) {
        vertexItem->setBrush(Qt::red);  // Đặt lại màu cho các đỉnh
    }
    coloredVertices.clear();  // Xóa danh sách các đỉnh đã tô màu

    for (QGraphicsLineItem* edgeItem : coloredEdges) {
        edgeItem->setPen(QPen(Qt::blue, 5));  // Đặt lại màu cho các cạnh
    }
    coloredEdges.clear();  // Xóa danh sách các cạnh đã tô màu

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
