import java.io.IOException;
//import org.apache.hadoop.conf.Context;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.hbase.mapreduce.TableMapReduceUtil;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.mapreduce.TableReducer;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;

/**
 * Puts a quadtree into hbase.
 * Takes in the text output.
 */
public class Putter {
    public static class InsertReducer extends TableReducer<Text, Text, Text> {
        public void reduce(Text key, Iterable<Text> values, Context context) throws IOException, InterruptedException {
            Put put = new Put(Bytes.toBytes(key.toString()));
            // will values be an iterable or csv string?
            for(Text species : values) {
                put.add(Bytes.toBytes("species"), Bytes.toBytes(species.toString()), null);
            }
            context.write(key, put);
        }
    }
 
    public static void main(String[] args) throws Exception {
        HBaseConfiguration conf = new HBaseConfiguration();
        Job job = new Job(conf, "Quadtree Inserter");
        job.setJarByClass(Putter.class);
        job.setMapperClass(Mapper.class);
        TableMapReduceUtil.initTableReducerJob("quadtree", InsertReducer.class, job);
        System.exit(job.waitForCompletion(true) ? 0 : 1);
    }
}
