#ifndef MATRIX_LOAD_PROGRESS_INCLUDE
#define MATRIX_LOAD_PROGRESS_INCLUDE

#define LPACCURACY  100

#define  LP_PRELOADROBOTS           0
#define  LP_LOADINGMAP              1
#define  LP_PREPARINGOBJECTS        2
#define  LP_PREPARININTERFACE       3

struct SLoadProcessProps
{
    const wchar *description;
    int len;

};

class CLoadProgress : public CMain
{
    int m_CurLoadProcess;
    int m_fullsize;
    float m_fullsize1;
    int m_cursizedone;
    int m_cur_lp_size;
    float m_cur_lp_size1;


    int m_lastacc;

public:
    CLoadProgress(void);
    ~CLoadProgress(void) {}

    void SetCurLP(int lp);
    void InitCurLP(int sz) {m_cur_lp_size = sz; m_cur_lp_size1 = 1.0f/float(sz);};
    void SetCurLPPos(int i);

};



#endif