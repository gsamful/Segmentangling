#include "SimplifyCT.hpp"

#include <cassert>
#include <QDebug>
#include <QFile>
#include <fstream>
#include <QTextStream>

namespace contourtree {

bool BranchCompare::operator() (uint32_t v1, uint32_t v2) {
     return sim->compare(v1,v2);
}

SimplifyCT::SimplifyCT() {
    queue = std::priority_queue<uint32_t,std::vector<uint32_t>,BranchCompare>(BranchCompare(this));
    order.clear();
}

void SimplifyCT::setInput(ContourTreeData *data) {
    this->data = data;
}

void SimplifyCT::addToQueue(uint32_t ano) {
    if(isCandidate(branches[ano])) {
        queue.push(ano);
        inq[ano] = true;
    }
}

bool SimplifyCT::isCandidate(const Branch &br) {
    uint32_t from = br.from;
    uint32_t to = br.to;
    if(nodes[from].prev.size() == 0) {
        // minimum
        if(nodes[to].prev.size() > 1) {
            return true;
        } else {
            return false;
        }
    }
    if(nodes[to].next.size() == 0) {
        // maximum
        if(nodes[from].next.size() > 1) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

void SimplifyCT::initSimplification(SimFunction* f) {
    branches.resize(data->noArcs);
    nodes.resize(data->noNodes);
    for(uint32_t i = 0;i < branches.size();i ++) {
        branches[i].from = data->arcs[i].from;
        branches[i].to = data->arcs[i].to;
        branches[i].parent = -1;
        branches[i].arcs.push_back(i);

        nodes[branches[i].from].next.push_back(i);
        nodes[branches[i].to].prev.push_back(i);
    }

    fn.resize(branches.size());
    removed.resize(branches.size(),false);
    invalid.resize(branches.size(),false);
    inq.resize(branches.size(), false);

    vArray.resize(nodes.size());

    simFn = f;
    if(f != NULL) {
        simFn->init(fn, branches);

        for(uint32_t i = 0;i < branches.size();i ++) {
            addToQueue(i);
        }
    }
}


bool SimplifyCT::compare(uint32_t b1, uint32_t b2) const {
    // If I want smallest weight on top, I need to return true if b1 > b2 (sort in descending order)
    if(fn[b1] > fn[b2]) {
        return true;
    }
    if(fn[b1] < fn[b2]) {
        return false;
    }
    float p1 = data->fnVals[branches[b1].to] - data->fnVals[branches[b1].from];
    float p2 = data->fnVals[branches[b2].to] - data->fnVals[branches[b2].from];
    if(p1 > p2) {
        return true;
    }
    if(p1 < p2) {
        return false;
    }
    int diff1 = branches[b1].to - branches[b1].from;
    int diff2 = branches[b2].to - branches[b2].from;
    if(diff1 > diff2) {
        return true;
    }
    if(diff1 < diff2) {
        return false;
    }
    return (branches[b2].from > branches[b1].from);
}

void SimplifyCT::removeArc(uint32_t ano) {
    Branch br = branches[ano];
    uint32_t from = br.from;
    uint32_t to = br.to;
    uint32_t mergedVertex = -1;
    if(nodes[from].prev.size() == 0) {
        // minimum
        mergedVertex = to;
    }
    if(nodes[to].next.size() == 0) {
        // maximum
        mergedVertex = from;
    }
    nodes[from].next.removeAll(ano);
    nodes[to].prev.removeAll(ano);
    removed[ano] = true;

    vArray[mergedVertex].push_back(ano);
    if(nodes[mergedVertex].prev.size() == 1 && nodes[mergedVertex].next.size() == 1) {
        mergeVertex(mergedVertex);
    }
    if(simFn != NULL)
        simFn->branchRemoved(branches, ano, invalid);
}

void SimplifyCT::mergeVertex(uint32_t v) {
    uint32_t prev = nodes[v].prev.at(0);
    uint32_t next = nodes[v].next.at(0);
    int a = -1;
    int rem = -1;
    if(inq[prev]) {
        invalid[prev] = true;
        removed[next] = true;
        branches[prev].to = branches[next].to;
        a = prev;
        rem = next;

        for(int i = 0;i < nodes[branches[prev].to].prev.size();i ++) {
            if(nodes[branches[prev].to].prev[i] == next) {
                nodes[branches[prev].to].prev[i] = prev;
            }
        }
    } else {
        invalid[next] = true;
        removed[prev] = true;
        branches[next].from = branches[prev].from;
        a = next;
        rem = prev;

        for(int i = 0;i < nodes[branches[next].from].next.size();i ++) {
            if(nodes[branches[next].from].next[i] == prev) {
                nodes[branches[next].from].next[i] = next;
            }
        }
        if(simFn != NULL && !inq[next]) {
            addToQueue(next);
        }
    }
    for(int i = 0;i < branches[rem].children.size();i ++) {
        int ch = branches[rem].children.at(i);
        branches[a].children.push_back(ch);
        assert(branches[ch].parent == rem);
        branches[ch].parent = a;
    }
    branches[a].arcs << branches[rem].arcs;
    for(int i = 0;i < vArray[v].size();i ++) {
        uint32_t aa = vArray[v].at(i);
        branches[a].children.push_back(aa);
        branches[aa].parent = a;
    }
    branches[rem].parent = -2;
}

void SimplifyCT::simplify(SimFunction *simFn) {
    qDebug() << "init";
    initSimplification(simFn);

    qDebug() << "going over priority queue";
    while(queue.size() > 0) {
        uint32_t ano = queue.top();
        queue.pop();
        inq[ano] = false;
        if(!removed[ano]) {
            if(invalid[ano]) {
                simFn->update(branches, ano);
                invalid[ano] = false;
                addToQueue(ano);
            } else {
                if(isCandidate(branches[ano])) {
                    removeArc(ano);
                    order.push_back(ano);
                }
            }
        }
    }
    qDebug() << "pass over removed";
    int root = 0;
    for(int i = 0;i < removed.size();i ++) {
        if(!removed[i]) {
            assert(root == 0);
            order.push_back(i);
            root ++;
        }
    }
}


void SimplifyCT::simplify(const std::vector<uint32_t> &order, int topk, float th, const std::vector<float> &wts) {
    qDebug() << "init";
    initSimplification(NULL);

    qDebug() << "going over order queue";
    for(int i = 0;i < order.size();i ++) {
        inq[order.at(i)] = true;
    }
    if(topk > 0) {
        size_t ct = order.size() - topk;
        for(int i = 0;i < ct;i ++) {
            uint32_t ano = order.at(i);
            if(!isCandidate(branches[ano])) {
                qDebug() << "failing candidate test";
                assert(false);
            }
            inq[ano] = false;
            removeArc(ano);
        }
    } else if(th != 0) {
        for(int i = 0;i < order.size() - 1;i ++) {
            uint32_t ano = order.at(i);
            if(!isCandidate(branches[ano])) {
                qDebug() << "failing candidate test";
                assert(false);
            }
            float fn = wts.at(i);
            if(fn > th) {
                break;
            }
            inq[ano] = false;
            removeArc(ano);
        }
    }
}

void SimplifyCT::outputOrder(QString fileName) {
    qDebug() << "Writing meta data";
    {
        QFile pr(fileName + ".order.dat");
        if(!pr.open(QFile::WriteOnly | QIODevice::Text)) {
            qDebug() << "could not write to file" << fileName + ".order.dat";
        }
        QTextStream text(&pr);
        text << order.size() << "\n";
        pr.close();
    }
    std::vector<float> wts;
    float pwt = 0;
    for(size_t i = 0;i < order.size();i ++) {
        uint32_t ano = order.at(i);
        float val = this->simFn->getBranchWeight(ano);
        wts.push_back(val);
        if(i > 0) {
            assert(pwt <= val);
        }
        pwt = val;
    }

    // normalize weights
    float maxWt = wts.at(wts.size() - 1);
    if(maxWt == 0) maxWt = 1;
    for(int i = 0;i < wts.size();i ++) {
        qDebug() << wts[i];
        wts[i] /= maxWt;
    }

    qDebug() << "writing tree output";
    QString binFile = fileName + ".order.bin";
    std::ofstream of(binFile.toStdString(),std::ios::binary);
    of.write((char *)order.data(),order.size() * sizeof(uint32_t));
    of.write((char *)wts.data(),wts.size() * sizeof(float));
//    of.write((char *)arcs.data(),arcs.size() * sizeof(uint32_t));
    of.close();
}

}
