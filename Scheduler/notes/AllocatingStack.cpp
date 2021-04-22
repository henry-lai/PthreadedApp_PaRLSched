void* PreFaultStack()
{
    const size_t NUM_PAGES_TO_PRE_FAULT = 50;
    const size_t size = NUM_PAGES_TO_PRE_FAULT * numa_pagesize();
    void *allocaBase = alloca(size);
    memset(allocaBase, 0, size);
    return allocaBase;
}

void SetAffinityAndRelocateStack(int cpuNum)
{
    assert(-1 != cpuNum);
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpuNum, &cpuset);
    const int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    assert(0 == rc);

    pthread_attr_t attr;
    void *stackAddr = nullptr;
    size_t stackSize = 0;
    if ((0 != pthread_getattr_np(pthread_self(), &attr)) || (0 != pthread_attr_getstack(&attr, &stackAddr, &stackSize))) {
        assert(false);
    }

    const unsigned long nodeMask = 1UL << numa_node_of_cpu(cpuNum);
    const auto bindRc = mbind(stackAddr, stackSize, MPOL_BIND, &nodeMask, sizeof(nodeMask), MPOL_MF_MOVE | MPOL_MF_STRICT);
    assert(0 == bindRc);

    PreFaultStack();
    // TODO: Also lock the stack with mlock() to guarantee it stays resident in RAM
    return;
}