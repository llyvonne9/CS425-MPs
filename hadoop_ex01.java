public static class MapClass extends MapReduceBase implements
Mapper<LongWriteable, Text, Text, Intwritable>{
	pribate final static IntWritable one = new IntWritable(1);
	private Text word = new Text();

	public void map(LongWritable key, Text value,
		OutputCollector<Text, IntWritable> output, Reporter reporter){
		// key is empty, value is the line
		throws IOException {
			String line = value.toString();
			stringTokenizer itr = new StringTOkenizer(line);
			while(itr.hasMoreTokens()){
				word.set(itr.nextToken());
				output.colect(word, one);
			}
		}
	}
}

public static class MapClass extends MapReduceBase implements
Reducer<Text, IntWritable, Text, IntWritable>{
	public void reduce(
		Text key,
		Iterator<Inwritable> values,
		OutputCollector<Text, IntWritable> output,
		Reporter reporter)
		throws IOException {
			//key is word, values is a list of 1's
			int sum = 0;
			while (values.has Next()){
				sum += values.next().get();
			}
			output.collect(key, new IntWritable(sum));
		}
}

//Tells Hadoop how to run your Map-Reduce job
public void run (String inputPath, String outputPath) throws Exception{
	//The job. WOrdCount contains MapClass and Reduce.
	JobConf conf = new JobConf(WordCount.class);
	conf.setJobName("mywordcount");
	//The keys are words
	(strings) conf.setOutputKeyClass(Text.class);
	//The values are counts (ints)
	conf.setOutputValueClass(IntWritable.class);
	conf.setMapperClass(MapClass.class);
	conf.setReducerClass(ReduceClass.class);
	FileInputFormat.addInputPath(
		conf, newPath(inputPath));
	FileOutputFormat.setOutputPath(
		conf, new Path(outputPath));
	JobClient.runJob(conf);
}