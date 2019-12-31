#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <ctime>
#include <queue>

using namespace std;

//定义城市
struct node {
    int num, x, y;
};

//定义个体
struct solution {
    vector<int> path;
    double sum, F, P;
    int gen;
};


int curGen = 1; //当前代数
bool isFound = false;   //是否找到最优解
const int maxGen = 10000; //最大代数
const int start = 1;    //起点
int packNum = 20;    //种群容量
double mutationP = 0.04;   //变异概率
double crossP = 0.7;     //交叉概率
double curCost = 0; //当前记录的最优解，用于传代统计
int Hayflick = 6;   //最大传代次数
vector<node> city;  //城市集合
vector<int> num;    //城市序列，用于shuffle
vector<solution> pack;  //种群
unordered_map<int, unordered_map<int, double> > dis;    //记录城市两两之间的距离
queue<double> record;   //传代统计队列

regex pattern("^(\\d+) (\\d+) (\\d+)$"); //读入数据的正则表达式

//计算两个城市之间的距离
double distanceBetween(node &a, node &b) {
    int x = a.x - b.x;
    int y = a.y - b.y;
    return ceil(sqrt((x * x + y * y) / 10.0));
}

//计算个体的解
double sum(vector<int> &v) {
    double sum = 0;
    for (auto it = v.begin() + 1; it != v.end(); it++) {
        sum += dis[*(it - 1)][*it];
    }
    return sum;
}

//比较器
bool cmp(const solution &a, const solution &b) {
    return a.sum < b.sum;
}

//初始化数据
void initData() {
    ifstream f("att48.tsp", ios::in);
    /*
     * att48.tsp format
     * number coordinate_x coordinate_y
    */

    if (!f) {
        cout << "File Not Found" << endl;
    }
    string s;
    while (getline(f, s)) {
        //筛选符合pattern的行
        if (regex_match(s, pattern)) {
            sregex_iterator it(s.begin(), s.end(), pattern);
            city.push_back(node{stoi(it->str(1)), stoi(it->str(2)), stoi(it->str(3))});
        }
    }
    f.close();

    //计算距离，存入unordered_map
    for (auto it = city.begin(); it != city.end(); it++) {
        unordered_map<int, double> temp;
        for (auto iterator = city.begin(); iterator != city.end(); iterator++) {
            temp[iterator->num] = distanceBetween(*it, *iterator);
        }
        dis[it->num] = temp;
    }

    for (auto it = city.begin(); it != city.end(); it++) {
        if (it->num != start) {
            num.push_back(it->num);
        }
    }
}

//初始化种群
void initPack(int gen) {
    for (int i = 0; i < packNum; i++) {
        //生成随机序列
        solution temp;
        temp.path.push_back(start);
        random_shuffle(num.begin(), num.end());
        for (auto it = num.begin(); it != num.end(); it++) {
            temp.path.push_back(*it);
        }
        temp.path.push_back(start);

        //计算个体的解并将其压入种群
        temp.gen = gen;
        temp.sum = sum(temp.path);
        pack.push_back(temp);
    }
    if (gen == 1) {
        solution greed;
        unordered_map<int, bool> isVisited;
        greed.path.push_back(start);
        isVisited[start] = true;
        for (int i = 0; i < num.size(); i++) {
            double min = -1;
            for (auto it = dis[greed.path.back()].begin(); it != dis[greed.path.back()].end(); it++) {
                if (isVisited.count(it->first) == 0 && (min == -1 || it->second < dis[greed.path.back()][min])) {
                    min = it->first;
                }
            }
            greed.path.push_back(min);
            isVisited[min] = true;
        }
        greed.path.push_back(start);
        greed.gen = gen;
        greed.sum = sum(greed.path);
        pack[0] = greed;
    }
}

//传代
void passOn() {
    record.push(pack[0].sum);
    //对每一代的最优解进行记录，如果超过1000代相同则进行传代操作
    if (record.size() > 500) {
        record.pop();
        //传代后第一次变化前不进行传代，避免过早指数爆炸
        if (curCost != pack[0].sum && record.front() == record.back()) {
            //cout << "Pass On!" << endl;
            vector<int> Prometheus = pack[0].path;
            int gen = pack[0].gen;
            double sum = pack[0].sum;
            pack.clear();
            //在最大传代限制前进行种群扩增
            if (Hayflick > 0) {
                Hayflick--;
                packNum *= log(packNum) / log(10);
            }
            //重新初始化种群，并将火种压入
            initPack(gen);
            pack[0].path = Prometheus;
            pack[0].sum = sum;
            sort(pack.begin(), pack.end(), cmp);
            curCost = sum;
            mutationP += 0.1;
            //清空记录
            while (!record.empty()) {
                record.pop();
            }
        }
    }
}

//交叉算子
solution cross(solution &firstParent, solution &secondParent) {
    //随机选择长度与起点
    int length = int((rand() % 1000 / 1000.0) * city.size());
    int off = rand() % city.size() + 1;
    vector<int> nextGen(firstParent.path.size());
    unordered_map<int, bool> selected;
    nextGen[0] = start;
    for (int i = off; i < nextGen.size() - 1 && i < off + length; i++) {
        nextGen[i] = firstParent.path[i];
        selected[nextGen[i]] = true;
    }
    for (int i = 1, j = 1; i < nextGen.size(); i++) {
        if (nextGen[i] == 0) {
            for (; j < secondParent.path.size(); j++) {
                if (selected.count(secondParent.path[j]) == 0) {
                    nextGen[i] = secondParent.path[j];
                    selected[secondParent.path[j]] = true;
                    break;
                }
            }
        }
    }
    return solution{nextGen, sum(nextGen), 0, 0, firstParent.gen + 1};
}

//变异算子 - 顺序移位 - 优化速度
void mutation(solution &cur) {
    //随机选择长度与起点
    int length = int((rand() % 1000 / 1000.0) * city.size());
    int off = rand() % city.size() + 1;
    vector<int> m;
    unordered_map<int, bool> selected;
    m.push_back(start);
    for (int i = off; i < cur.path.size() - 1 && i < off + length; i++) {
        m.push_back(cur.path[i]);
        selected[cur.path[i]] = true;
    }
    for (int i = 1; i < cur.path.size(); i++) {
        if (selected.count(cur.path[i]) == 0) {
            m.push_back(cur.path[i]);
        }
    }
    for (int i = 0; i < m.size(); i++) {
        cur.path[i] = m[i];
    }
    cur.sum = sum(cur.path);
}

//变异算子 - 贪婪倒位 - 优化效率
void gmutation(solution &cur) {
    //随机选择起点，找到距起点最近的点，然后将最近点到起点之间的段倒位
    int selected = rand() % (city.size() - 4) + 2, min = 1;
    int selectedCity = cur.path[selected];
    int begin = 0, end = 0;
    for (auto it = dis[selectedCity].begin(); it != dis[selectedCity].end(); it++) {
        if (it->first != selectedCity && it->second < dis[selectedCity][min]) {
            min = it->first;
        }
    }
    for (int i = 1; i < cur.path.size() - 1; i++) {
        if (cur.path[i] == min) {
            if (i > selected + 1) {
                begin = selected + 1;
                end = i;
            } else if (i < selected - 1) {
                begin = i;
                end = selected - 1;
            }
            break;
        }
    }
    vector<int> stack;
    for (int i = begin; i <= end; i++) {
        stack.push_back(cur.path[i]);
    }
    for (int i = begin; i <= end; i++) {
        cur.path[i] = stack.back();
        stack.pop_back();
    }
    cur.sum = sum(cur.path);
}

//进化过程
vector<solution> process() {
    double total = 0;   //总适应度
    vector<solution> nextGenPack;   //下一代种群
    sort(pack.begin(), pack.end() - 1, cmp);    //排序找出最优个体
    printf("%d %d\n", pack[0].gen, (int) pack[0].sum);
    if (pack[0].sum == 10628) {
        isFound = true;
    }
    passOn();   //传代操作
    //计算种群种每个个体的适应度
    for (auto it = pack.begin(); it != pack.end() - 1; it++) {
        it->F = 10000 / it->sum;
        it->P = (it == pack.begin() ? 0 : (it - 1)->P) + it->F;
        total += it->F;
    }
    //最优个体直接进入下一代，防止反向进化
    nextGenPack.push_back(pack[0]);
    nextGenPack[0].gen++;
    //应用轮盘赌，选择交叉个体
    //由于种群容量不变且最优个体占位，故每一代保留最优个体的同时，剔除最差个体
    //即最差个体不参与轮盘赌交叉
    for (auto firstParent = pack.begin(); firstParent != pack.end() - 1; firstParent++) {
        if (rand() % 10000 / 10000.0 < crossP) {
            double selected = (rand() % 10000 / 10000.0) * total;
            for (auto secondParent = pack.begin(); secondParent != pack.end() - 1; secondParent++) {
                if (selected < secondParent->P) {
                    nextGenPack.push_back(cross(*firstParent, *secondParent));
                    break;
                }
            }
        } else {
            firstParent->gen++;
            nextGenPack.push_back(*firstParent);
        }
        if (rand() % 10000 / 10000.0 < mutationP) {
            //当传代1次后变更算子到更高效的贪婪倒位
            Hayflick < 6 ? gmutation(nextGenPack.back()) : mutation(nextGenPack.back());
        }
    }
    return nextGenPack;
}

int main() {
    clock_t start = clock();
    srand(unsigned(time(NULL)));    //设置时间种子
    initData();     //初始化数据
    initPack(1);    //初始化种群
    while (!isFound && curGen <= maxGen) {
        pack = process();
        curGen++;
    }
    cout << "Total time: " << double(clock() - start) / CLOCKS_PER_SEC << endl;
    cout << "The best: " << pack[0].sum << endl;
    for (auto it = pack[0].path.begin(); it != pack[0].path.end(); it++) {
        cout << *it << " ";
    }
    return 0;
}
