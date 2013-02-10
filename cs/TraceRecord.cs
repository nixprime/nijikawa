namespace Nijikawa
{
    class TraceRecord
    {
        public ulong Addr { get; set; }
        public ulong Prec { get; set; }
        public bool IsWrite { get; set; }
    }
}
