namespace Nijikawa
{
    class Simulator
    {
        public long Now { get; private set; }

        public Simulator()
        {
            Now = 0;
        }

        public void Tick()
        {
            Now++;
        }
    }
}
