import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Partitioner;

public class QuadtreePartitioner<K, V> extends Partitioner <K, V> {
    /** 
     * Partitions based on the quadtree address of the key.
     * Quadtree address: w.x.y.z.... (where x,y... are ints)
     * Currently partitions based on the most 
     * significant piece (furthest to left).
     * TODO: support partitioning on further pieces of the address.
     * This assumes numPartitions == 4
     * @see Partitioner.getPartition
     */
    @Override
    public int getPartition(K key, V value, int numPartitions) {
        assert(numPartitions == 4);
        String quadrant = key.toString().substring(0, 1);
        int partition = Integer.parseInt(quadrant);
        return partition;
    }
}
