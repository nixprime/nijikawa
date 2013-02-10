using System;
using System.Globalization;
using System.IO;

namespace Nijikawa
{
    class UsimmTraceReader : ITraceReader
    {
        private StreamReader Reader { get; set; }

        public UsimmTraceReader(string filename)
        {
            Reader = new StreamReader(filename);
        }

        TraceRecord ITraceReader.NextTraceRecord()
        {
            var line = Reader.ReadLine();
            var words = line.Split(' ');
            if (words.Length != 3 && words.Length != 4)
            {
                throw new ApplicationException("Invalid trace file");
            }
            var record = new TraceRecord();
            record.Prec = uint.Parse(words[0]);
            record.Addr = ulong.Parse(words[2].Substring(2),
                    NumberStyles.HexNumber);
            if (words[1] == "R")
            {
                record.IsWrite = false;
            }
            else if (words[1] == "W")
            {
                record.IsWrite = true;
            }
            else
            {
                throw new ApplicationException("Unknown request type " +
                        words[1]);
            }
            return record;
        }
    }
}
