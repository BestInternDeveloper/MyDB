//
// Created by Zhengxiao Du on 2018/10/29.
//

#include "RM_FileScan.h"
#include "RM_FileHandle.h"
#include "../pf/pf.h"
#include "../parser/Expr.h"

int
RM_FileScan::openScan(const RM_FileHandle &fileHandle, Expr *condition, const std::string &tableName) {
    _file_handle = &fileHandle;
    _condition = condition;
    _current_page = 0;
    _current_bitdata = nullptr;
    this->tableName = tableName;
    return 0;
}


int RM_FileScan::getNextRec(RM_Record &rec) {
    while (true) {
        unsigned slot_num = _file_handle->_header_page.recordPerPage + 1;
        if (_current_page != 0) {
            slot_num = _current_bitmap.findLeftOne();
        }
        while (slot_num >= _file_handle->_header_page.recordPerPage) {
            _current_page += 1;
            if (_current_page >= _file_handle->_header_page.pageNum) {
                return 1;
            }
            PF_PageHandle page_handle;
            _file_handle->_pf_file_handle.GetThisPage(_current_page, page_handle);
            char *page_data;
            page_handle.GetData(page_data);
            delete[]_current_bitdata;
            _current_bitdata = new char[_file_handle->_header_page.slotMapSize];
            memcpy(_current_bitdata, page_data + 8, _file_handle->_header_page.slotMapSize);
            _file_handle->_pf_file_handle.UnpinPage(_current_page);
            for (int i = 0; i < _file_handle->_header_page.slotMapSize; ++i) {
                _current_bitdata[i] = ~_current_bitdata[i];
            }
            _current_bitmap = MyBitMap(_file_handle->_header_page.slotMapSize * 8,
                                       reinterpret_cast<unsigned *>(_current_bitdata));
            slot_num = _current_bitmap.findLeftOne();
        }
        _file_handle->getRec(RID{_current_page, slot_num}, rec);
        _current_bitmap.setBit(slot_num, 0);
        char *data = rec.getData();
        if (_condition != nullptr) {
            _condition->init_calculate(tableName);
            _condition->calculate(data, this->tableName);
            if (not _condition->calculated or _condition->is_true()) {
                _condition->init_calculate(tableName);
                break;
            }
        } else {
            break;
        }

    }
    return 0;
}

int RM_FileScan::closeScan() {
    delete[] _current_bitdata;
    _current_bitdata = nullptr;
    return 0;
}

int
RM_FileScan::openScan(const RM_FileHandle &fileHandle, AttrType attrType, int attrLength, int attrOffset, CompOp compOp,
                      void *value) {
    if (compOp != CompOp::NO_OP) {
        Expr *left = new Expr();
        left->attrInfo.attrSize = attrLength;
        left->attrInfo.attrOffset = attrOffset;
        left->attrInfo.attrType = attrType;
        left->attrInfo.notNull = false;
        left->nodeType = NodeType::ATTR_NODE;

        Expr *right;
        if (value != nullptr) {
            switch (attrType) {
                case AttrType::INT:
                case AttrType::DATE: {
                    int i = *reinterpret_cast<int *>(value);
                    right = new Expr(i);
                    break;
                }
                case AttrType::FLOAT: {
                    float f = *reinterpret_cast<float *>(value);
                    right = new Expr(f);
                    break;
                }
                case AttrType::STRING: {
                    char *s = reinterpret_cast<char *>(value);
                    right = new Expr(s);
                    break;
                }
                case AttrType::BOOL: {
                    bool b = *reinterpret_cast<bool *>(value);
                    right = new Expr(b);
                    break;
                }
                case AttrType::NO_ATTR:
                case AttrType::VARCHAR:
                    right = new Expr();
                    break;
            }
        } else {
            right = new Expr();
        }

        Expr *condition = new Expr(left, compOp, right);
        return openScan(fileHandle, condition, "");
    } else {
        return openScan(fileHandle, nullptr, "");
    }
}

RM_FileScan::~RM_FileScan() {
    closeScan();
}
