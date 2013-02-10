namespace Nijikawa
{
    enum MemRequestType
    {
        Read,
        Write,
    }

    class MemRequest
    {
        public MemRequestType Type { get; private set; }
        public ulong Addr { get; private set; }
        private ResponseReceiverDel ResponseReceiver { get; set; }

        public delegate void ResponseReceiverDel(long cycle,
                MemResponse response);

        public MemRequest(MemRequestType type, ulong addr,
                ResponseReceiverDel responseReceiver)
        {
            Type = type;
            Addr = addr;
            ResponseReceiver = responseReceiver;
        }

        public void Respond(long cycle) {
            if (ResponseReceiver != null) {
                ResponseReceiver(cycle, new MemResponse(Addr));
            }
        }
    }

    class MemResponse
    {
        public ulong Addr { get; private set; }

        public MemResponse(ulong addr)
        {
            this.Addr = addr;
        }
    }

    interface IMemRequestReceiver
    {
        void ReceiveMemRequest(MemRequest req);
    }
}
